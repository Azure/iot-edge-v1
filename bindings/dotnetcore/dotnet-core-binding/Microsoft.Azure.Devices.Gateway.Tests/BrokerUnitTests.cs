using System;
using Xunit;
using Microsoft.Azure.Devices.Gateway;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Moq;

namespace Microsoft.Azure.Devices.Gateway.Tests
{
    public class BrokerUnitTests
    {
        /* Tests_SRS_DOTNET_CORE_BROKER_04_001: [ If broker is <= 0, Broker constructor shall throw a new ArgumentException ] */
        [Fact]
        public void Broker_Constructor_WhenBrokerLowerThanZero_ShouldThrowArgumentOutOfRange()
        {
            ///arrage
            IntPtr invalidBroker = IntPtr.Zero;
            IntPtr validSourceModule = (IntPtr)0x42;

            ///act
            try
            {
                var brokerInstance = new Broker(invalidBroker, validSourceModule, new BrokerInterop());
            }
            catch (ArgumentOutOfRangeException e)
            {
                ///assert
                Assert.Contains("Invalid broker", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");
            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_BROKER_04_007: [ If module is <= 0, Broker constructor shall throw a new ArgumentException ] */
        [Fact]
        public void Broker_Constructor_WhenmoduleHandleLowerThanZero_ShouldThrowArgumentOutOfRange()
        {
            ///arrage
            IntPtr broker = (IntPtr)0x42;
            IntPtr sourceModule = IntPtr.Zero;

            ///act
            try
            {
                var brokerInstance = new Broker(broker, sourceModule, new BrokerInterop());
            }
            catch (ArgumentOutOfRangeException e)
            {
                ///assert
                Assert.Contains("Invalid source module", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_BROKER_04_002: [ If broker and module are greater than 0, Broker constructor shall save this value and succeed. ] */
        [Fact]
        public void Broker_Constructor_succeed()
        {
            ///arrage
            IntPtr broker = (IntPtr)0x42;
            IntPtr sourceModule = (IntPtr)0x42;

            ///act
            var brokerInstance = new Broker(broker, sourceModule, new BrokerInterop());

            Assert.NotNull(brokerInstance);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_BROKER_04_004: [ Publish shall not catch exception thrown by ToByteArray. ] */
        [Fact]
        public void Broker_Publish_Does_not_Catch_Exception_by_MessageToByteArray()
        {
            ///arrage
            IntPtr broker = (IntPtr)0x42;
            IntPtr sourceModule = (IntPtr)0x42;
            var brokerInstance = new Broker(broker, sourceModule, new BrokerInterop());
            Mock<Message> myMessageToPublish = new Mock<Message>(new byte[0], new Dictionary<string, string>());

            myMessageToPublish.Setup(t => t.ToByteArray()).Throws(new FormatException("Fake Exception."));

            ///act
            try
            {
                brokerInstance.Publish(myMessageToPublish.Object);
            }
            catch (FormatException e)
            {
                Assert.Contains("Fake Exception.", e.Message);
                myMessageToPublish.Verify(t => t.ToByteArray());
                return;
            }

            //assert
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_BROKER_04_006: [ If Module_DotNetHost_PublishMessage fails, Publish shall thrown an Application Exception with message saying that Broker Publish failed. ] */
        [Fact]
        public void Broker_Publish_throws_when_interop_fail()
        {
            ///arrage
            IntPtr broker = (IntPtr)0x42;
            IntPtr sourceModule = (IntPtr)0x42;
            Mock<Message> myMessageToPublish = new Mock<Message>(new byte[0], new Dictionary<string, string>());
            Mock<BrokerInterop> myBrokerInteropMock = new Mock<BrokerInterop>();
            var brokerInstance = new Broker(broker, sourceModule, myBrokerInteropMock.Object);

            myBrokerInteropMock.Setup(t => t.PublishMessage((IntPtr)broker, (IntPtr)broker, It.IsAny<byte[]>())).Throws(new Exception("Fake Exception."));

            ///act
            try
            {
                brokerInstance.Publish(myMessageToPublish.Object);
            }
            catch (Exception e)
            {
                //assert
                myMessageToPublish.Verify(t => t.ToByteArray());
                myBrokerInteropMock.Verify(t => t.PublishMessage((IntPtr)broker, (IntPtr)broker, It.IsAny<byte[]>()));
                Assert.Contains("Failed to publish message.", e.Message);
                return;
            }

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_BROKER_04_003: [ Publish shall call the Message.ToByteArray() method to get the Message object translated to byte array. ] */
        /* Tests_SRS_DOTNET_CORE_BROKER_04_005: [ Publish shall call the native method Module_DotNetHost_PublishMessage passing the broker and module value saved by it's constructor, the byte[] got from Message and the size of the byte array. ] */
        [Fact]
        public void Broker_Publish_succeed()
        {
            ///arrage
            IntPtr broker = (IntPtr)0x42;
            IntPtr sourceModule = (IntPtr)0x42;
            Mock<Message> myMessageToPublish = new Mock<Message>(new byte[0], new Dictionary<string, string>());
            Mock<BrokerInterop> myBrokerInteropMock = new Mock<BrokerInterop>();
            var brokerInstance = new Broker(broker, sourceModule, myBrokerInteropMock.Object);

            ///act
            brokerInstance.Publish(myMessageToPublish.Object);

            //assert
            myMessageToPublish.Verify(t => t.ToByteArray());
            myBrokerInteropMock.Verify(t => t.PublishMessage((IntPtr)broker, (IntPtr)broker, It.IsAny<byte[]>()));


            ///cleanup
        }
    }
}
