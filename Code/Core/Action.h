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

    // ��ϵ� ��� �Լ� ȣ��
    void Invoke(Types... args) {
        for (auto& func : mFunctions) {
            func(args...);
        }
    }

    void Clear() {
        mFunctions.clear();
    }
private:
    std::vector<std::function<void(Types...)>> mFunctions; // ȣ�� ������ �Լ� ���
};
