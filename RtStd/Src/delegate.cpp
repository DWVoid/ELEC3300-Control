#include <thread.hpp>
#include <delegate.hpp>
#include <semaphore.hpp>

namespace {
    struct ForwardNode {
    	void(*Function)(void*) noexcept;
    	void* User;
    	rstd::thread::id Id;
    	ForwardNode* Next;
    };

    ForwardNode* gHead = nullptr, *gTail = nullptr;

    class Exec {
    public:
    	static Exec& Get() noexcept {
    		static Exec instance {};
    		return instance;
    	}

    	void Signal() noexcept { gSignal.release(); }
    private:
    	Exec() noexcept {
    		rstd::thread(osPriorityHigh, 4096, [this]() noexcept {
    			for (;;) {
    				gSignal.acquire();
    				const auto& ths = *gHead;
    				gHead = gHead->Next;
    				ths.Function(ths.User);
    				osThreadResume(ths.Id.native());
    			}
    		}).detach();
    	}

    	rstd::semaphore gSignal { 4096, 0 };
    };
}

void rstd::_delegate_run(void(*function)(void*) noexcept, void* user) noexcept {
	ForwardNode node {
		function, user,
		rstd::this_thread::get_id(),
		nullptr
	};
	if (gHead == nullptr) {
		gHead = gTail = &node;
	}
	else {
		gTail->Next = &node;
		gTail = &node;
	}
	Exec::Get().Signal();
	osThreadSuspend(node.Id.native());
}

void rstd::hold_indefinitely() noexcept { for(;;) osDelay(0x7FFFFFFF); }
