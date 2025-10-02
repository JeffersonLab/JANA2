
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <iterator>
#include <unistd.h>

#include "JFactorySet.h"
#include "JANA/Utils/JTablePrinter.h"
#include "JFactory.h"

//---------------------------------
// JFactorySet    (Constructor)
//---------------------------------
JFactorySet::JFactorySet(void)
{

}

//---------------------------------
// ~JFactorySet    (Destructor)
//---------------------------------
JFactorySet::~JFactorySet()
{
    // Deleting the factories will clear their databundles but not delete them
    for (auto* factory : mFactories) delete factory;

    // Databundles are always owned by the factoryset and always deleted here
    for (auto* databundle : mDatabundles) {
        delete databundle;
    }
}

//---------------------------------
// Add
//---------------------------------
void JFactorySet::Add(JDatabundle* databundle) {
    if (databundle->GetUniqueName().empty()) {
        throw JException("Attempted to add a databundle with no unique_name");
    }
    auto named_result = mDatabundleFromUniqueName.find(databundle->GetUniqueName());
    if (named_result != std::end(mDatabundleFromUniqueName)) {
        // Collection is duplicate. Since this almost certainly indicates a user error, and
        // the caller will not be able to do anything about it anyway, throw an exception.
        // We show the user which factory is causing this problem, including both plugin names

        auto ex = JException("Attempted to add duplicate databundles");
        ex.function_name = "JFactorySet::Add";
        ex.instance_name = databundle->GetUniqueName();

        auto fac = databundle->GetFactory();
        if (fac != nullptr) {
            ex.type_name = fac->GetTypeName();
            ex.plugin_name = fac->GetPluginName();
            if (named_result->second->GetFactory() != nullptr) {
                ex.plugin_name += ", " + named_result->second->GetFactory()->GetPluginName();
            }
        }
        throw ex;
    }

    mDatabundles.push_back(databundle);
    mDatabundleFromUniqueName[databundle->GetUniqueName()] = databundle;
    mDatabundleFromTypeIndexAndEitherName[{databundle->GetTypeIndex(), databundle->GetUniqueName()}] = databundle;
    mDatabundleFromTypeIndexAndEitherName[{databundle->GetTypeIndex(), databundle->GetShortName()}] = databundle;
    mDatabundleFromTypeNameAndEitherName[{databundle->GetTypeName(), databundle->GetUniqueName()}] = databundle;
    mDatabundleFromTypeNameAndEitherName[{databundle->GetTypeName(), databundle->GetShortName()}] = databundle;
    mDatabundlesFromTypeIndex[databundle->GetTypeIndex()].push_back(databundle);
    mDatabundlesFromTypeName[databundle->GetTypeName()].push_back(databundle);
}

//---------------------------------
// Add
//---------------------------------
void JFactorySet::Add(JFactory* factory)
{
    /// Add a JFactory to this JFactorySet. The JFactorySet assumes ownership of this factory.
    /// If the JFactorySet already contains a JFactory with the same key,
    /// throw an exception and let the user figure out what to do.
    /// This scenario occurs when the user has multiple JFactory<T> producing the
    /// same T JObject, and is not distinguishing between them via tags.
    /// Returns bool indicating whether the add succeeded.

    if (factory->GetLevel() != mLevel && mLevel != JEventLevel::None && factory->GetLevel() != JEventLevel::None) {
        //LOG << "    Skipping factory with type_name=" << factory->GetTypeName()
        //    << ", level=" << toString(factory->GetLevel())
        //    << " to event with level= " << toString(mLevel);
        delete factory;
        return;
    }
    /*
    else {
        LOG << "    Adding factory with type_name=" << factory->GetTypeName()
            << ", level=" << toString(factory->GetLevel())
            << " to event with level= " << toString(mLevel);
    }
    */

    mFactories.push_back(factory);

    for (auto* output : factory->GetOutputs()) {
        if (output->GetLevel() != mLevel && output->GetLevel() != JEventLevel::None) {
            throw JException("Factory outputs are required to be at the same level as the factory itself");
        }
        auto* databundle = output->GetDatabundle();
        databundle->SetFactory(factory); // It's a little weird to set this here
        Add(databundle);
    }
    for (auto* variadic_output : factory->GetVariadicOutputs()) {
        if (variadic_output->GetLevel() != mLevel && variadic_output->GetLevel() != JEventLevel::None) {
            throw JException("Factory outputs are required to be at the same level as the factory itself");
        }
        for (const auto& databundle : variadic_output->GetDatabundles()) {
            databundle->SetFactory(factory); // It's a little weird to set this here
            Add(databundle);
        }
    }
}

//---------------------------------
// GetDatabundle
//---------------------------------
JDatabundle* JFactorySet::GetDatabundle(const std::string& unique_name) const {
    auto it = mDatabundleFromUniqueName.find(unique_name);
    if (it != std::end(mDatabundleFromUniqueName)) {
        return it->second;
    }
    return nullptr;
}

//---------------------------------
// GetDatabundle
//---------------------------------
JDatabundle* JFactorySet::GetDatabundle(const std::string& object_type_name, const std::string& unique_or_short_name) const {
    auto it = mDatabundleFromTypeNameAndEitherName.find({object_type_name, unique_or_short_name});
    if (it != std::end(mDatabundleFromTypeNameAndEitherName)) {
        return it->second;
    }
    return nullptr;
}

//---------------------------------
// GetDatabundle
//---------------------------------
JDatabundle* JFactorySet::GetDatabundle(std::type_index object_type_index, const std::string& unique_or_short_name) const {
    auto it = mDatabundleFromTypeIndexAndEitherName.find({object_type_index, unique_or_short_name});
    if (it != std::end(mDatabundleFromTypeIndexAndEitherName)) {
        return it->second;
    }
    return nullptr;
}

//---------------------------------
// GetDatabundles
//---------------------------------
const std::vector<JDatabundle*>& JFactorySet::GetDatabundles(std::type_index index) const {
    static std::vector<JDatabundle*> no_databundles {};
    auto it = mDatabundlesFromTypeIndex.find(index);
    if (it != std::end(mDatabundlesFromTypeIndex)) {
        return it->second;
    }
    return no_databundles;
}

//---------------------------------
// Print
//---------------------------------
void JFactorySet::Print() const {

    JTablePrinter table;
    table.AddColumn("Factory type name");
    table.AddColumn("Factory prefix");
    table.AddColumn("Databundle type name");
    table.AddColumn("Databundle unique name");
    table.AddColumn("Status");
    table.AddColumn("Size");

    for (auto* factory : mFactories) {
        for (auto* output: factory->GetOutputs()) {
            auto* databundle = output->GetDatabundle();
            table | (factory->GetTypeName().empty() ? "[Unset]" : factory->GetTypeName());
            table | factory->GetPrefix();
            table | databundle->GetTypeName();
            table | databundle->GetUniqueName();
            switch (databundle->GetStatus()) {
                case JDatabundle::Status::Empty:    table | "Empty";    break;
                case JDatabundle::Status::Created:  table | "Created";  break;
                case JDatabundle::Status::Inserted: table | "Inserted"; break;
                case JDatabundle::Status::Excepted: table | "Excepted"; break;
            }
            table | databundle->GetSize();
        }
        for (auto* output: factory->GetVariadicOutputs()) {
            for (auto* databundle: output->GetDatabundles()) {
                table | (factory->GetTypeName().empty() ? "[Unset]" : factory->GetTypeName());
                table | factory->GetPrefix();
                table | databundle->GetTypeName();
                table | databundle->GetUniqueName();
                switch (databundle->GetStatus()) {
                    case JDatabundle::Status::Empty:    table | "Empty";    break;
                    case JDatabundle::Status::Created:  table | "Created";  break;
                    case JDatabundle::Status::Inserted: table | "Inserted"; break;
                    case JDatabundle::Status::Excepted: table | "Excepted"; break;
                }
                table | databundle->GetSize();
            }
        }
    }
    for (auto* databundle : mDatabundles) {
        if (databundle->GetFactory() == nullptr) {
            table | "[None]";
            table | "[None]";
            table | databundle->GetTypeName();
            table | databundle->GetUniqueName();
            switch (databundle->GetStatus()) {
                case JDatabundle::Status::Empty:    table | "Empty";    break;
                case JDatabundle::Status::Created:  table | "Created";  break;
                case JDatabundle::Status::Inserted: table | "Inserted"; break;
                case JDatabundle::Status::Excepted: table | "Excepted"; break;
            }
            table | databundle->GetSize();
        }
    }
    table.Render(std::cout);
}

//---------------------------------
// Clear
//---------------------------------
void JFactorySet::Clear() {

    for (auto* factory : mFactories) {
        factory->ClearData();
    }
    for (auto* databundle : mDatabundles) {
        // Clear any databundles that did not come from a JFactory
        if (databundle->GetFactory() == nullptr) {
            databundle->ClearData();
        }
    }
}

//---------------------------------
// Finish
//---------------------------------
void JFactorySet::Finish() {
    for (auto& factory : mFactories) {
        factory->DoFinish();
    }
}

