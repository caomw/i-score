#pragma once
#include <iscore/command/SerializableCommand.hpp>
#include <iscore/tools/ObjectPath.hpp>

#include <tests/helpers/ForwardDeclaration.hpp>
class DeckModel;
namespace Scenario
{
    namespace Command
    {
        /**
         * @brief The AddDeckToBox class
         *
         * Adds an empty deck at the end of a Box.
         */
        class AddDeckToBox : public iscore::SerializableCommand
        {
                ISCORE_COMMAND
#include <tests/helpers/FriendDeclaration.hpp>
            public:
                ISCORE_SERIALIZABLE_COMMAND_DEFAULT_CTOR(AddDeckToBox, "ScenarioControl")
                AddDeckToBox(ObjectPath&& boxPath);

                virtual void undo() override;
                virtual void redo() override;

            protected:
                virtual void serializeImpl(QDataStream&) const override;
                virtual void deserializeImpl(QDataStream&) override;

            private:
                ObjectPath m_path;

                id_type<DeckModel> m_createdDeckId {};
        };
    }
}