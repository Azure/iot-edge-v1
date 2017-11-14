using System.Reflection;

namespace Microsoft.Azure.Devices.Gateway
{
    /// <summary>
    ///   Object that represents a .NET Core module instance.
    /// </summary>
    internal class DotNetCoreModuleInstance
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
