#include "ProcessViewModelInterfaceSerialization.hpp"
#include "ProcessSharedModelInterface.hpp"
#include "ProcessViewModelInterface.hpp"
#include "Document/Constraint/ConstraintModel.hpp"

#include <interface/serialization/DataStreamVisitor.hpp>
#include <interface/serialization/JSONVisitor.hpp>

template<>
void Visitor<Reader<DataStream>>::readFrom(const ProcessViewModelInterface& processViewModel)
{
	// To allow recration using createProcessViewModel.
	// This supposes that the process is stored inside a Constraint.
	m_stream << processViewModel.sharedProcessModel()->id();

	readFrom(static_cast<const IdentifiedObject&>(processViewModel));

	// ProcessViewModelInterface doesn't have any particular data to save

	// Save the subclass
	processViewModel.serialize(DataStream::type(),
							   static_cast<void*>(this));
}

template<>
ProcessViewModelInterface* createProcessViewModel(Deserializer<DataStream>& deserializer,
												  ConstraintModel* constraint,
												  QObject* parent)
{
	SettableIdentifier sharedProcessId;
	deserializer.m_stream >> sharedProcessId;

	auto process = constraint->process((SettableIdentifier::identifier_type) sharedProcessId);
	auto viewmodel = process->makeViewModel(DataStream::type(),
											static_cast<void*>(&deserializer),
											parent);

	return viewmodel;
}




template<>
void Visitor<Reader<JSON>>::readFrom(const ProcessViewModelInterface& processViewModel)
{
	// To allow recration using createProcessViewModel.
	// This supposes that the process is stored inside a Constraint.
	m_obj["SharedProcessId"] = toJsonObject(processViewModel.sharedProcessModel()->id());

	readFrom(static_cast<const IdentifiedObject&>(processViewModel));

	// ProcessViewModelInterface doesn't have any particular data to save

	// Save the subclass
	processViewModel.serialize(JSON::type(),
							   static_cast<void*>(this));
}

template<>
ProcessViewModelInterface* createProcessViewModel(Deserializer<JSON>& deserializer,
												  ConstraintModel* constraint,
												  QObject* parent)
{
	SettableIdentifier sharedProcessId;
	fromJsonObject(deserializer.m_obj["SharedProcessId"].toObject(), sharedProcessId);

	auto process = constraint->process((SettableIdentifier::identifier_type) sharedProcessId);
	auto viewmodel = process->makeViewModel(JSON::type(),
											static_cast<void*>(&deserializer),
											parent);

	return viewmodel;
}