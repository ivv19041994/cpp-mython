#include "runtime.h"

#include <cassert>
#include <optional>
#include <sstream>
#include <functional>

#include <iostream>

using namespace std;

namespace runtime {

namespace {
    const string EQUAL_METHOD = "__eq__"s;
    const string LESS_METHOD = "__lt__"s;
    const string TO_STRING_METHOD = "__str__"s;
}  // namespace

ObjectHolder::ObjectHolder(std::shared_ptr<Object> data)
    : data_(std::move(data)) {
}

void ObjectHolder::AssertIsValid() const {

    if (data_ == nullptr) {
        assert(false);
    }

    assert(data_ != nullptr);
}

ObjectHolder ObjectHolder::Share(Object& object) {
    // Возвращаем невладеющий shared_ptr (его deleter ничего не делает)
    return ObjectHolder(std::shared_ptr<Object>(&object, [](auto* /*p*/) { /* do nothing */ }));
}

ObjectHolder ObjectHolder::None() {
    return ObjectHolder();
}

Object& ObjectHolder::operator*() const {
    AssertIsValid();
    return *Get();
}

Object* ObjectHolder::operator->() const {
    AssertIsValid();
    return Get();
}

Object* ObjectHolder::Get() const {
    return data_.get();
}

ObjectHolder::operator bool() const {
    return Get() != nullptr;
}

bool IsTrue(const ObjectHolder& object) {
    if(!object) {
        return false;
    }
    
    if(String *str = object.TryAs<String>()) {
        return str->GetValue().size() != 0;
    }
    
    if(Number *num = object.TryAs<Number>()) {
        return num->GetValue() != 0;
    }
    
    if(Bool *b = object.TryAs<Bool>()) {
        return b->GetValue();
    }
    return false;
}

    
void ClassInstance::Print(std::ostream& os, Context& context) {
    if(HasMethod(TO_STRING_METHOD, 0)) {
        ObjectHolder retval = ClassInstance::Call(TO_STRING_METHOD,
                                         {},
                                         context);
        retval->Print(os, context);
        return;
    }
    
    os << this;
}

bool ClassInstance::HasMethod(const std::string& method, size_t argument_count) const {
    const Method* pmethod = cls_.GetMethod(method);
    if(pmethod == nullptr) {
        return false;
    }
    if(pmethod->formal_params.size() != argument_count) {
        return false;
    }
    return true;
}

Closure& ClassInstance::Fields() {
    return fields_;
}

const Closure& ClassInstance::Fields() const {
    return fields_;
}

ClassInstance::ClassInstance(const Class& cls): cls_{cls} {
    
}

ObjectHolder ClassInstance::Call(const std::string& method,
                                 const std::vector<ObjectHolder>& actual_args,
                                 Context& context) {
    if(!HasMethod(method, actual_args.size())) {
        throw std::runtime_error("Method: "s + method + " does not exist"s);
    }
    const Method* pmethod = cls_.GetMethod(method);
    Closure filds;
    for(size_t i = 0; i < actual_args.size(); ++i) {
        assert(actual_args[i].Get());
        filds[pmethod->formal_params[i]] = actual_args[i];
    }
    filds["self"] = ObjectHolder::Share(*this);
    return pmethod->body->Execute(filds, context);
}

Class::Class(std::string name, std::vector<Method> methods, const Class* parent)
: name_{std::move(name)}
, parent_{parent} {
    for(Method& method : methods) {
        methods_[method.name] = std::move(method);
    }
}

const Method* Class::GetMethod(const std::string& name) const {
    if(methods_.count(name) == 0) {
        if(parent_) {
            return parent_->GetMethod(name);
        }
        return nullptr;
    }
    return &methods_.at(name);
}

const std::string& Class::GetName() const {
    return name_;
}

void Class::Print(ostream& os, Context& /*context*/) {
    os << "Class " << GetName();
}

void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context) {
    os << (GetValue() ? "True"sv : "False"sv);
}

template <typename CompareFunc>
bool Compare(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context, const std::string method_name, CompareFunc comparator) {

    #define COMPARE_VALUE_OBJ(TYPE) \
    if(TYPE* lstr = lhs.TryAs<TYPE>()) {\
        if(TYPE* rstr = rhs.TryAs<TYPE>()) {\
            return comparator(lstr->GetValue(), rstr->GetValue());\
        }\
    }
    COMPARE_VALUE_OBJ(String)
    COMPARE_VALUE_OBJ(Bool)
    COMPARE_VALUE_OBJ(Number)
    #undef COMPARE_VALUE_OBJ

    if (ClassInstance* lclass_instance = lhs.TryAs<ClassInstance>()) {
        assert(rhs);
        Bool* b = lclass_instance->Call(method_name,
            { ObjectHolder::Share(*rhs.Get()) },
            context).TryAs<Bool>();
        if (b) {
            return b->GetValue();
        }
    }

    throw std::runtime_error("Invalid compare call"s);
}

bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    
    if(!lhs && !rhs) {
        return true;
    }

    return Compare(lhs, rhs, context, EQUAL_METHOD, std::equal_to<>{});
}

bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return Compare(lhs, rhs, context, LESS_METHOD, std::less<>{});
}

bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Equal(lhs, rhs, context);
}

bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    if(Less(lhs, rhs, context)) {
        return false;
    }
    
    return !Equal(lhs, rhs, context);
}

bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return Less(lhs, rhs, context) || Equal(lhs, rhs, context);
}

bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Less(lhs, rhs, context);
}

}  // namespace runtime