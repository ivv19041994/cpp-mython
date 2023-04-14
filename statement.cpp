#include "statement.h"

#include <iostream>
#include <sstream>
#include <cassert>

using namespace std;

namespace ast {

using runtime::Closure;
using runtime::Context;
using runtime::ObjectHolder;
using runtime::ClassInstance;
using runtime::Method;
    

namespace {
const string ADD_METHOD = "__add__"s;
const string INIT_METHOD = "__init__"s;
const string SUB_METHOD = "__sub__"s;
const string MUL_METHOD = "__mul__"s;
const string DIV_METHOD = "__truediv__"s;
const string NONE = "None"s;
}  // namespace

void PrintObjectHolder(const ObjectHolder& obj, Context& context) {
    if (obj) {
        obj.Get()->Print(context.GetOutputStream(), context);
    }
    else {
        context.GetOutputStream() << NONE;
    }
}
    
std::ostream& operator<<(std::ostream& os, const ObjectHolder& obj) {
    runtime::SimpleContext sc{os};
    PrintObjectHolder(obj, sc);
    return os;
}

ObjectHolder Assignment::Execute(Closure& closure, Context& context) {
    ObjectHolder ret = rv_->Execute(closure, context);
    closure[var_] = ret;
    return ret;
}

Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv)
: var_{std::move(var)}
, rv_{std::move(rv)} {
}

VariableValue::VariableValue(const std::string& var_name)
: dotted_ids_{var_name} {
    assert(var_name.find('.') == var_name.npos);
}

VariableValue::VariableValue(std::vector<std::string> dotted_ids)
: dotted_ids_{std::move(dotted_ids)} {
    assert(dotted_ids_.size() > 0);
}

ObjectHolder VariableValue::Execute(Closure& closure, Context& /*context*/) {
    Closure::const_iterator it = closure.find(dotted_ids_[0]);
    
    if(it == closure.end()) {
        throw std::runtime_error("Unknown fild " + dotted_ids_[0]);
    }
    
    for(size_t i = 1; i < dotted_ids_.size(); ++i) {
        const ClassInstance* pclass_instance = it->second.TryAs<ClassInstance>();
        assert(pclass_instance != nullptr);
        const Closure& fields = pclass_instance->Fields();
        it = fields.find(dotted_ids_[i]);
        if(it == fields.end()) {
            throw std::runtime_error("Unknown fild " + dotted_ids_[i]);
        }
    }
    
    return it->second;
}

unique_ptr<Print> Print::Variable(const std::string& name) {
    return make_unique<Print>(make_unique<VariableValue>(name));
}

Print::Print(unique_ptr<Statement> argument) {
    args_.emplace_back(std::move(argument));
}

Print::Print(vector<unique_ptr<Statement>> args)
: args_{std::move(args)} {
}

ObjectHolder Print::Execute(Closure& closure, Context& context) {
    if(args_.size()) {
        PrintObjectHolder(args_[0]->Execute(closure, context), context);
    }
    for(size_t i = 1; i < args_.size(); ++i) {
        context.GetOutputStream() << " ";
        PrintObjectHolder(args_[i]->Execute(closure, context), context);
    }
    context.GetOutputStream() << "\n";
    
    return {};
}

MethodCall::MethodCall(std::unique_ptr<Statement> object, std::string method,
                       std::vector<std::unique_ptr<Statement>> args)
: object_{std::move(object)}
, method_{std::move(method)}
, args_{std::move(args)} {
}

ObjectHolder MethodCall::Execute(Closure& closure, Context& context) {
    auto obj = object_->Execute(closure, context);
    if(ClassInstance* class_instance = obj.TryAs<ClassInstance>()) {
        std::vector<ObjectHolder> actual_args;
        for(auto& arg: args_) {
            actual_args.push_back(arg->Execute(closure, context));
        }
        return class_instance->Call(method_, actual_args, context);
    }
    throw std::runtime_error("Call method for not class type");
}
    
UnaryOperation::UnaryOperation(std::unique_ptr<Statement> argument)
: argument_{std::move(argument)}{
    
}

ObjectHolder Stringify::Execute(Closure& closure, Context& context) {
    std::stringstream ss;
    runtime::SimpleContext simple_context(ss);
    PrintObjectHolder(argument_->Execute(closure, context), simple_context);
    return ObjectHolder::Own(runtime::String{ss.str()});
}
    
BinaryOperation::BinaryOperation(std::unique_ptr<Statement> lhs, std::unique_ptr<Statement> rhs)
: lhs_{std::move(lhs)}
, rhs_{std::move(rhs)} {
    
}

ObjectHolder Add::Execute(Closure& closure, Context& context) {
    auto lhs = lhs_->Execute(closure, context);
    auto rhs = rhs_->Execute(closure, context);
    if(runtime::Number* lnumber = lhs.TryAs<runtime::Number>()) {
        if(runtime::Number* rnumber = rhs.TryAs<runtime::Number>()) {
            return ObjectHolder::Own(runtime::Number{lnumber->GetValue() + rnumber->GetValue()});
        }
    } else if(runtime::String* lstring = lhs.TryAs<runtime::String>()) {
        if(runtime::String* rstring = rhs.TryAs<runtime::String>()) {
            return ObjectHolder::Own(runtime::String{lstring->GetValue() + rstring->GetValue()});
        }
    } else if(ClassInstance* lclass_instance = lhs.TryAs<ClassInstance>()) {
        if(lclass_instance->HasMethod(ADD_METHOD, 1)) {
            std::vector<ObjectHolder> actual_args;
            actual_args.emplace_back(std::move(rhs));
            return lclass_instance->Call(ADD_METHOD, actual_args, context);
        }
    }
    throw std::runtime_error("Adding with diferent types");
}

ObjectHolder Sub::Execute(Closure& closure, Context& context) {
    auto lhs = lhs_->Execute(closure, context);
    auto rhs = rhs_->Execute(closure, context);
    if(runtime::Number* lnumber = lhs.TryAs<runtime::Number>()) {
        if(runtime::Number* rnumber = rhs.TryAs<runtime::Number>()) {
            return ObjectHolder::Own(runtime::Number{lnumber->GetValue() - rnumber->GetValue()});
        }
    } else if(ClassInstance* lclass_instance = lhs.TryAs<ClassInstance>()) {
        if(lclass_instance->HasMethod(SUB_METHOD, 1)) {
            std::vector<ObjectHolder> actual_args;
            actual_args.emplace_back(std::move(rhs));
            return lclass_instance->Call(SUB_METHOD, actual_args, context);
        }
    }
    throw std::runtime_error("Sub with diferent types");
}

ObjectHolder Mult::Execute(Closure& closure, Context& context) {
    auto lhs = lhs_->Execute(closure, context);
    auto rhs = rhs_->Execute(closure, context);
    if(runtime::Number* lnumber = lhs.TryAs<runtime::Number>()) {
        if(runtime::Number* rnumber = rhs.TryAs<runtime::Number>()) {
            return ObjectHolder::Own(runtime::Number{lnumber->GetValue() * rnumber->GetValue()});
        }
    } else if(ClassInstance* lclass_instance = lhs.TryAs<ClassInstance>()) {
        if(lclass_instance->HasMethod(MUL_METHOD, 1)) {
            std::vector<ObjectHolder> actual_args;
            actual_args.emplace_back(std::move(rhs));
            return lclass_instance->Call(MUL_METHOD, actual_args, context);
        }
    }
    throw std::runtime_error("Mult with diferent types");
}

ObjectHolder Div::Execute(Closure& closure, Context& context) {
    auto lhs = lhs_->Execute(closure, context);
    auto rhs = rhs_->Execute(closure, context);
    if(runtime::Number* lnumber = lhs.TryAs<runtime::Number>()) {
        if(runtime::Number* rnumber = rhs.TryAs<runtime::Number>()) {
            if(rnumber->GetValue()) {
                return ObjectHolder::Own(runtime::Number{lnumber->GetValue() / rnumber->GetValue()});
            } else {
                throw std::runtime_error("Div0");
            }
                
        }
    } else if(ClassInstance* lclass_instance = lhs.TryAs<ClassInstance>()) {
        if(lclass_instance->HasMethod(DIV_METHOD, 1)) {
            std::vector<ObjectHolder> actual_args;
            actual_args.emplace_back(std::move(rhs));
            return lclass_instance->Call(DIV_METHOD, actual_args, context);
        }
    }
    throw std::runtime_error("Div with diferent types");
}
    
void Compound::AddStatement(std::unique_ptr<Statement> stmt) {
    instructions_.emplace_back(std::move(stmt));
}

ObjectHolder Compound::Execute(Closure& closure, Context& context) {
    for(auto& instruction: instructions_) {
        if (dynamic_cast<Return*>(instruction.get())) {
            return instruction->Execute(closure, context);
        }

        if (dynamic_cast<IfElse*>(instruction.get())) {
            auto result = instruction->Execute(closure, context);
            if (result) {
                return result;
            }
        }
        else {
            instruction->Execute(closure, context);
        }
    }
    return {};
}

Return::Return(std::unique_ptr<Statement> statement)
: statement_{std::move(statement)} {
        
}

ObjectHolder Return::Execute(Closure& closure, Context& context) {
    return statement_->Execute(closure, context);
}

ClassDefinition::ClassDefinition(ObjectHolder cls)
: cls_{cls} {
}

ObjectHolder ClassDefinition::Execute(Closure& closure, Context& /*context*/) {
    runtime::Class* _class = cls_.TryAs<runtime::Class>();
    assert(_class);
    closure[_class->GetName()] = cls_;
    return {};
}

FieldAssignment::FieldAssignment(VariableValue object, std::string field_name,
                                 std::unique_ptr<Statement> rv)
: object_{std::move(object)}
, field_name_{std::move(field_name)}
, rv_{std::move(rv)} {
    
}

ObjectHolder FieldAssignment::Execute(Closure& closure, Context& context) {
    ObjectHolder var = object_.Execute(closure, context);
    ClassInstance *pclass_instance = var.TryAs<ClassInstance>();
    assert(pclass_instance);
    
    return pclass_instance->Fields()[field_name_] = rv_->Execute(closure, context);
}

IfElse::IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
               std::unique_ptr<Statement> else_body)
: condition_{std::move(condition)}
, if_body_{std::move(if_body)}
, else_body_{std::move(else_body)} {
}

ObjectHolder IfElse::Execute(Closure& closure, Context& context) {
    auto condition = condition_->Execute(closure, context);
    if(IsTrue(condition)) {
        return if_body_->Execute(closure, context);
        
    }
    if(else_body_) {
        return else_body_->Execute(closure, context);
    }

    return {};
}

ObjectHolder Not::Execute(Closure& closure, Context& context) {
    auto argument = argument_->Execute(closure, context);
    if(runtime::Bool* b = argument.TryAs<runtime::Bool>()) {
        return ObjectHolder::Own(runtime::Bool{!b->GetValue()});
    }
    
    throw std::runtime_error("not for not bool val");
}

Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
    : BinaryOperation(std::move(lhs), std::move(rhs))
    , cmp_{std::move(cmp)}{
}

ObjectHolder Comparison::Execute(Closure& closure, Context& context) {
    bool result = cmp_(
        lhs_->Execute(closure, context),
        rhs_->Execute(closure, context),
        context
    );
    return ObjectHolder::Own(runtime::Bool(result));
}

NewInstance::NewInstance(const runtime::Class& local_class, std::vector<std::unique_ptr<Statement>> args)
: class_{local_class}
, args_{std::move(args)} {
}

NewInstance::NewInstance(const runtime::Class& local_class)
: class_{local_class} {

}

ObjectHolder NewInstance::Execute(Closure& closure, Context& context) {

    ObjectHolder class_instance = ObjectHolder::Own(ClassInstance{ class_ });
    
    const Method* init = class_.GetMethod(INIT_METHOD);
    size_t params = (init == nullptr) ? 0 : init->formal_params.size();
    
    if(params != args_.size()) {
        throw std::runtime_error("Can't find constructor for " + class_.GetName());
    }
    
    if(init) {
        std::vector<ObjectHolder> actual_args;
        for(size_t i = 0; i < params; ++i) {
            actual_args.push_back(args_[i]->Execute(closure, context));
        }
        
        class_instance.TryAs<ClassInstance>()->Call(INIT_METHOD, actual_args, context);
    }
    
    return class_instance;
}

MethodBody::MethodBody(std::unique_ptr<Statement>&& body) : body_{ std::move(body) } {
}

ObjectHolder MethodBody::Execute(Closure& closure, Context& context) {

    return body_->Execute(closure, context);
}

}  // namespace ast