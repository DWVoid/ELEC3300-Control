#include <exception>

class MachineException: public std::exception {
public:
	MachineException(const char* str) noexcept: mText(str) {}

	[[nodiscard]] const char* what() const noexcept override { return mText; }
private:
	const char* mText;
};
