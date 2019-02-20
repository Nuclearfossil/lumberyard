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

#include "Vector3ParameterEditor.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/Parameter/Vector3Parameter.h>
#include <EMotionStudio/EMStudioSDK/Source/Allocators.h>
#include <MCore/Source/AttributeVector3.h>


namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(Vector3ParameterEditor, EMStudio::UIAllocator, 0)

    Vector3ParameterEditor::Vector3ParameterEditor(EMotionFX::AnimGraph* animGraph, const EMotionFX::ValueParameter* valueParameter, const AZStd::vector<MCore::Attribute*>& attributes)
        : ValueParameterEditor(animGraph, valueParameter, attributes)
        , m_currentValue(0.0f, 0.0f, 0.0f)
    {
        UpdateValue();
    }

    void Vector3ParameterEditor::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<Vector3ParameterEditor, ValueParameterEditor>()
            ->Version(1)
            ->Field("value", &Vector3ParameterEditor::m_currentValue)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<Vector3ParameterEditor>("Vector3 parameter editor", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &Vector3ParameterEditor::m_currentValue, "", "")
                ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &ValueParameterEditor::GetDescription)
                ->Attribute(AZ::Edit::Attributes::Min, &Vector3ParameterEditor::GetMinValue)
                ->Attribute(AZ::Edit::Attributes::Max, &Vector3ParameterEditor::GetMaxValue)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Vector3ParameterEditor::OnValueChanged)
                ->Attribute(AZ::Edit::Attributes::ReadOnly, &ValueParameterEditor::IsReadOnly)
        ;
    }

    void Vector3ParameterEditor::UpdateValue()
    {
        // Use the value from the first attribute, they all should match since they all are the same parameter in different graph instances
        if (!m_attributes.empty())
        {
            MCore::AttributeVector3* attribute = static_cast<MCore::AttributeVector3*>(m_attributes[0]);
            m_currentValue = AZ::Vector3(attribute->GetValue().GetX(), attribute->GetValue().GetY(), attribute->GetValue().GetZ());
        }
        else
        {
            const EMotionFX::Vector3Parameter* parameter = static_cast<const EMotionFX::Vector3Parameter*>(m_valueParameter);
            m_currentValue = parameter->GetDefaultValue();
        }
    }

    void Vector3ParameterEditor::setIsReadOnly(bool isReadOnly)
    {
        ValueParameterEditor::setIsReadOnly(isReadOnly);
    }

    AZ::Vector3 Vector3ParameterEditor::GetMinValue() const
    {
        const EMotionFX::Vector3Parameter* parameter = static_cast<const EMotionFX::Vector3Parameter*>(m_valueParameter);
        return parameter->GetMinValue();
    }

    AZ::Vector3 Vector3ParameterEditor::GetMaxValue() const
    {
        const EMotionFX::Vector3Parameter* parameter = static_cast<const EMotionFX::Vector3Parameter*>(m_valueParameter);
        return parameter->GetMaxValue();
    }

    void Vector3ParameterEditor::OnValueChanged()
    {
        for (MCore::Attribute* attribute : m_attributes)
        {
            MCore::AttributeVector3* typedAttribute = static_cast<MCore::AttributeVector3*>(attribute);
            typedAttribute->SetValue(AZ::PackedVector3f(m_currentValue.GetX(), m_currentValue.GetY(), m_currentValue.GetZ()));
        }
    }
}
