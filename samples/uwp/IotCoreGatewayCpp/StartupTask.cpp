// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pch.h"
#include "StartupTask.h"

using namespace IotCoreGatewayCpp;

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::ApplicationModel::Background;
using namespace Microsoft::Azure::IoT::Gateway;
using namespace SetOfCppModules;

// The Background Application template is documented at http://go.microsoft.com/fwlink/?LinkID=533884&clcid=0x409

void StartupTask::Run(IBackgroundTaskInstance^ taskInstance)
{
	deferral = taskInstance->GetDeferral();

	auto modules = ref new Vector<IGatewayModule^>();
	modules->Append(ref new Module2());
	modules->Append(ref new Module1());
	modules->Append(ref new Module3());
	
	gateway = ref new Gateway(modules);
}

