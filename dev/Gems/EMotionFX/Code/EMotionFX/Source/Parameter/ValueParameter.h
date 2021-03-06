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

#include "Parameter.h"
#include <MCore/Source/Config.h>

namespace MCore
{
    class Attribute;
}

namespace EMotionFX
{
    /**
     * ValueParameter inherits from Parameter and is the base type for all the parameters that contain a value (i.e. nOot groups)
     */
    class ValueParameter
        : public Parameter
    {
    public:
        
        AZ_RTTI(ValueParameter, "{46549C79-6B4C-4DDE-A5E3-E5FBEC455816}", Parameter)
        AZ_CLASS_ALLOCATOR_DECL

        ~ValueParameter() override = default;

        static void Reflect(AZ::ReflectContext* context);

        // Methods required to support MCore::Attribute
        virtual MCore::Attribute* ConstructDefaultValueAsAttribute() const = 0;
        virtual uint32 GetType() const = 0;
        virtual bool AssignDefaultValueToAttribute(MCore::Attribute* attribute) const = 0;
        virtual bool SetDefaultValueFromAttribute(MCore::Attribute* attribute) = 0;
        virtual bool SetMinValueFromAttribute(MCore::Attribute* attribute) { return false; }
        virtual bool SetMaxValueFromAttribute(MCore::Attribute* attribute) { return false; }
    };

    typedef AZStd::vector<ValueParameter*> ValueParameterVector;
}
