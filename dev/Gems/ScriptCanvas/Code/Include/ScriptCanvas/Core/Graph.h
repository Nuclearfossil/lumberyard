/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/mutex.h>

#include <ScriptCanvas/AST/Node.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/GraphData.h>
#include <ScriptCanvas/Debugger/Bus.h>
#include <ScriptCanvas/Execution/ErrorBus.h>
#include <ScriptCanvas/Execution/ExecutionContext.h>
#include <ScriptCanvas/Execution/RuntimeBus.h>

namespace ScriptCanvas
{
    class Node;
    class Connection;

    //! TODO: Remove the execution logic from this class and change all ScriptCanvas UnitTest to use the Runtime Component
    //! Graph is the execution model of a ScriptCanvas graph.
    class Graph
        : public AZ::Component
        , protected GraphRequestBus::MultiHandler
        , protected RuntimeRequestBus::Handler
        , private AZ::EntityBus::Handler
    {
    public:
        friend Node;

        AZ_COMPONENT(Graph, "{C3267D77-EEDC-490E-9E42-F1D1F473E184}");

        static void Reflect(AZ::ReflectContext* context);

        Graph(const AZ::EntityId& uniqueId = AZ::Entity::MakeId());

        ~Graph() override;

        void Init() override;
        void Activate() override;
        void Deactivate() override;

        AZ::EntityId GetUniqueId() const { return m_uniqueId; };

        //// GraphRequestBus::Handler
        bool AddNode(const AZ::EntityId&) override;
        bool RemoveNode(const AZ::EntityId& nodeId);
        Node* FindNode(AZ::EntityId nodeID) const override;
        AZStd::vector<AZ::EntityId> GetNodes() const override;
        const AZStd::vector<AZ::EntityId> GetNodesConst() const;
        AZStd::unordered_set<AZ::Entity*>& GetNodeEntities() { return m_graphData.m_nodes; }
        const AZStd::unordered_set<AZ::Entity*>& GetNodeEntities() const { return m_graphData.m_nodes; }

        bool AddConnection(const AZ::EntityId&) override;
        bool RemoveConnection(const AZ::EntityId& nodeId) override;
        AZStd::vector<AZ::EntityId> GetConnections() const override;
        AZStd::vector<Endpoint> GetConnectedEndpoints(const Endpoint& firstEndpoint) const override;
        bool FindConnection(AZ::Entity*& connectionEntity, const Endpoint& firstEndpoint, const Endpoint& otherEndpoint) const override;

        bool Connect(const AZ::EntityId& sourceNodeId, const SlotId& sourceSlotId, const AZ::EntityId& targetNodeId, const SlotId& targetSlotId) override;
        bool Disconnect(const AZ::EntityId& sourceNodeId, const SlotId& sourceSlotId, const AZ::EntityId& targetNodeId, const SlotId& targetSlotId) override;

        bool ConnectByEndpoint(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) override;
        AZ::Outcome<void, AZStd::string> CanConnectByEndpoint(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) const override;
        bool DisconnectByEndpoint(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) override;
        bool DisconnectById(const AZ::EntityId& connectionId) override;

        //! Retrieves the Entity this Graph component is currently located on
        //! NOTE: There can be multiple Graph components on the same entity so calling FindComponent may not not return this GraphComponent
        AZ::Entity* GetGraphEntity() const override { return GetEntity(); }

        Graph* GetGraph() { return this; }

        GraphData* GetGraphData() override { return &m_graphData; }
        const GraphData* GetGraphDataConst() const override { return &m_graphData; }

        bool AddGraphData(const GraphData&) override;
        void RemoveGraphData(const GraphData&) override;

        AZStd::unordered_set<AZ::Entity*> CopyItems(const AZStd::unordered_set<AZ::Entity*>& entities) override;
        void AddItems(const AZStd::unordered_set<AZ::Entity*>& graphField) override;
        void RemoveItems(const AZStd::unordered_set<AZ::Entity*>& graphField) override;
        void RemoveItems(const AZStd::vector<AZ::Entity*>& graphField);
        AZStd::unordered_set<AZ::Entity*> GetItems() const override;

        bool AddItem(AZ::Entity* itemRef) override;
        bool RemoveItem(AZ::Entity* itemRef) override;
        ///////////////////////////////////////////////////////////

        bool IsInErrorState() const { return m_executionContext.IsInErrorState(); }
        bool IsInIrrecoverableErrorState() const { return m_executionContext.IsInErrorState(); }
        AZStd::string_view GetLastErrorDescription() const { return m_executionContext.GetLastErrorDescription(); }
    protected:
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ScriptCanvasRuntimeService", 0x776e1e3a));;
        }

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ScriptCanvasService", 0x41fd58f3));
        }

        bool ValidateConnectionEndpoints(const AZ::EntityId& connectionRef, const AZStd::unordered_set<AZ::EntityId>& nodeRefs);

        AZ::Outcome<void, AZStd::string> ValidateDataFlow(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) const;
        AZ::Outcome<void, AZStd::string> ValidateDataFlow(AZ::Entity& sourceNodeEntity, AZ::Entity& targetNodeEntity, const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) const;

        bool IsInDataFlowPath(const Node* sourceNode, const Node* targetNode) const;

        void RefreshDataFlowValidity(bool warnOnRemoval = false);

        //// RuntimeRequestBus::Handler
        AZ::EntityId GetRuntimeEntityId() const override { return GetEntity() ? GetEntityId() : AZ::EntityId(); }

        VariableData* GetVariableData() override;
        const VariableData* GetVariableDataConst() const { return const_cast<Graph*>(this)->GetVariableData(); }
        const AZStd::unordered_map<VariableId, VariableNameValuePair>* GetVariables() const override;
        VariableDatum* FindVariable(AZStd::string_view propName) override;
        VariableNameValuePair* FindVariableById(const VariableId& variableId) override;
        Data::Type GetVariableType(const VariableId& variableId) const override;
        AZStd::string_view GetVariableName(const VariableId& variableId) const override;
        ////

    private:
        AZ::EntityId m_uniqueId;
        GraphData m_graphData;
        ExecutionContext m_executionContext;

        void OnEntityActivated(const AZ::EntityId&) override;
        class GraphEventHandler;
    };
}