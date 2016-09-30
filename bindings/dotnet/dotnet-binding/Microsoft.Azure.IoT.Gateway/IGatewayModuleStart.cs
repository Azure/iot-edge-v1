namespace Microsoft.Azure.IoT.Gateway
{
    /// <summary> Optional Start Interface to be implemented by the .NET Module </summary>
    public interface IGatewayModuleStart
    {
        /// <summary>
        ///     Informs module the gateway is ready to send and receive messages.
        /// </summary>
        /// <returns></returns>
        void Start();
    }
}