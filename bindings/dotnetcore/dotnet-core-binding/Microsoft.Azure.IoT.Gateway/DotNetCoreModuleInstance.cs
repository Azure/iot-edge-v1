using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Reflection;

namespace Microsoft.Azure.IoT.Gateway
{
    /// <summary>
    ///   Object that represents a .NET Core module instance.
    /// </summary>
    public class DotNetCoreModuleInstance
    {
        /// <summary>
        /// .NET Core module Instance.
        /// </summary>
        public IGatewayModule gatewayModule = null;

        /// <summary>
        /// Reference to the Receive method to be called upon receipt of a message
        /// </summary>
        public MethodInfo receiveMethodInfo = null;

        /// <summary>
        /// Reference to the Destroy method, to be called upon gateway destroy call.
        /// </summary>
        public MethodInfo destroyMethodInfo = null;


        /// <summary>
        /// Reference to the Start method, to be called upon gateway start.
        /// </summary>
        public MethodInfo startMethodInfo = null;
    }
}
