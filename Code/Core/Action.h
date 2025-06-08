#pragma once

template<typename... Types>
class Action
{
public:
    Action() {}
    ~Action() {}

    void operator+=(std::function<void(Types...)> func) {
        mFunctions.push_back(func);
    }

    // 등록된 모든 함수 호출
    void Invoke(Types... args) {
        for (auto& func : mFunctions) {
            func(args...);
        }
    }

    void Clear() {
        mFunctions.clear();
    }
private:
    std::vector<std::function<void(Types...)>> mFunctions; // 호출 가능한 함수 목록
};
