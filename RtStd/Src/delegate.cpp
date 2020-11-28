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

    	void Push(ForwardNode* node) noexcept {
    		if (gHead == nullptr) {
    			gHead = gTail = node;
    		}
    		else {
    			gTail->Next = node;
    			gTail = node;
    		}
    		gSignal.release();
    	}
    private:
    	Exec() noexcept {
    		rstd::thread(osPriorityHigh, 4096, [this]() noexcept {
    			for (;;) Once();
    		}).detach();
    	}

    	void Once() noexcept {
    		gSignal.acquire();
    		const auto& ths = *gHead;
    		gHead = gHead->Next;
    		ths.Function(ths.User);
    		if (ths.Id == rstd::thread::id()) delete &ths;
    		else osThreadResume(ths.Id.native());
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
	Exec::Get().Push(&node);
	osThreadSuspend(node.Id.native());
}


void rstd::_delegate_spawn(void(*function)(void*) noexcept, void* user) noexcept {
	Exec::Get().Push(new ForwardNode {
		function, user,
		rstd::thread::id(),
		nullptr
	});
}

void rstd::hold_indefinitely() noexcept { for(;;) osDelay(0x7FFFFFFF); }
