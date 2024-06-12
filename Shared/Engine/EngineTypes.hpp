#pragma once

#include <memory>
#include <string>
#include <cassert>
#include <iostream>
#include <vector>
#include <optional>

namespace mythSystem
{

    template<typename T>
    using UniquePtr = std::unique_ptr<T>;

    //template<typename T>
    //using MakeUnique = std::make_unique<T>;

    template<typename T>
    using SharedPtr = std::shared_ptr<T>;

    //template<typename T>
    //using MakeShared = std::make_shared<T>;

};// namespace mythSystem