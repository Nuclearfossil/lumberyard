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

#include "LmbrCentral_precompiled.h"
#include "SphereShapeComponent.h"

#include <AzCore/Math/IntersectPoint.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Shape/ShapeDisplay.h>

namespace LmbrCentral
{
    void SphereShape::Reflect(AZ::ReflectContext* context)
    {
        SphereShapeConfig::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SphereShape>()
                ->Version(1)
                ->Field("Configuration", &SphereShape::m_sphereShapeConfig)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SphereShape>("Sphere Shape", "Sphere shape configuration parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SphereShape::m_sphereShapeConfig, "Sphere Configuration", "Sphere shape configuration")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
            }
        }
    }

    void SphereShape::Activate(AZ::EntityId entityId)
    {
        m_entityId = entityId;
        m_currentTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_currentTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);

        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        ShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
        SphereShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
    }

    void SphereShape::Deactivate()
    {
        SphereShapeComponentRequestsBus::Handler::BusDisconnect();
        ShapeComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void SphereShape::InvalidateCache(InvalidateShapeCacheReason reason)
    {
        m_intersectionDataCache.InvalidateCache(reason);
    }

    void SphereShape::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentTransform = world;
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::TransformChange);
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);
    }

    void SphereShape::SetRadius(float radius)
    {
        m_sphereShapeConfig.m_radius = radius;
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    float SphereShape::GetRadius()
    {
        return m_sphereShapeConfig.m_radius;
    }

    AZ::Aabb SphereShape::GetEncompassingAabb()
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_sphereShapeConfig);

        return AZ::Aabb::CreateCenterRadius(m_currentTransform.GetPosition(), m_intersectionDataCache.m_radius);
    }

    bool SphereShape::IsPointInside(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_sphereShapeConfig);

        return AZ::Intersect::PointSphere(
            m_intersectionDataCache.m_position, powf(m_intersectionDataCache.m_radius, 2.0f), point);
    }

    float SphereShape::DistanceSquaredFromPoint(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_sphereShapeConfig);

        const AZ::Vector3 pointToSphereCenter = m_intersectionDataCache.m_position - point;
        const float distance = pointToSphereCenter.GetLength() - m_intersectionDataCache.m_radius;
        return powf(AZStd::max(distance, 0.0f), 2.0f);
    }

    bool SphereShape::IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, AZ::VectorFloat& distance)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_sphereShapeConfig);

        return AZ::Intersect::IntersectRaySphere(
            src, dir, m_intersectionDataCache.m_position, m_intersectionDataCache.m_radius, distance) > 0;
    }

    void SphereShape::SphereIntersectionDataCache::UpdateIntersectionParamsImpl(
        const AZ::Transform& currentTransform, const SphereShapeConfig& configuration)
    {
        m_position = currentTransform.GetPosition();
        m_radius = configuration.m_radius * currentTransform.RetrieveScale().GetMaxElement();
    }

    void DrawSphereShape(
        const ShapeDrawParams& shapeDrawParams, const SphereShapeConfig& boxConfig,
        AzFramework::EntityDebugDisplayRequests& displayContext)
    {
        if (shapeDrawParams.m_filled)
        {
            displayContext.SetColor(shapeDrawParams.m_shapeColor.GetAsVector4());
            displayContext.DrawBall(AZ::Vector3::CreateZero(), boxConfig.m_radius);
        }

        displayContext.SetColor(shapeDrawParams.m_wireColor.GetAsVector4());
        displayContext.DrawWireSphere(AZ::Vector3::CreateZero(), boxConfig.m_radius);
    }
} // namespace LmbrCentral