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

#include "StandardHeaders.h"
#include <AzCore/std/string/string.h>
#include <AzCore/std/function/function_fwd.h>

namespace MCore
{
    // forward declarations
    class CommandManager;

    /**
     * The file system class.
     * This currently contains some utility classes to deal with files on disk.
     */
    class MCORE_API FileSystem
    {
    public:
        /**
         * Save to file secured by a backup file.
         * @param[in] fileName The filename of the file.
         * @param[in] saveFunction Save function used to save the file.
         * @param[in] commandManager Command manager used to add error.
         * @result True in case everything went fine, false it something wrong happened. Check log for these cases.
         */
        static bool SaveToFileSecured(const char* filename, const AZStd::function<bool()>& saveFunction, CommandManager* commandManager = nullptr);

        static const char       mFolderSeparatorChar;   /**< The folder separator slash type used on the different supported platforms. */
        static AZStd::string    mSecureSavePath;        /**< The folder path used to keep a backup in SaveToFileSecured. */
    };
} // namespace MCore