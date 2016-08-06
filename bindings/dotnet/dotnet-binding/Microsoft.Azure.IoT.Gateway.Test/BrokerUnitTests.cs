// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.Azure.IoT.Gateway;
using Moq;
using System.Collections.Generic;

namespace Microsoft.Azure.IoT.Gateway.Test
{

    [TestClass]
    public class BrokerUnitTest
    {
        /* Tests_SRS_DOTNET_BROKER_04_001: [ If broker is <= 0, Broker constructor shall throw a new ArgumentException ] */
        [TestMethod]
        public void Broker_Constructor_WhenBrokerLowerThanZero_ShouldThrowArgumentOutOfRange()
        {
            ///arrage
            long invalidBroker = -1;
            long validSourceModule = 0x42;

            ///act
            try
            {
                var brokerInstance = new Broker(invalidBroker, validSourceModule);
            }
            catch (ArgumentOutOfRangeException e)
            {
                ///assert
                StringAssert.Contains(e.Message, "Invalid broker");
                return;
            }
            Assert.Fail("No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_BROKER_04_007: [ If module is <= 0, Broker constructor shall throw a new ArgumentException ] */
        [TestMethod]
        public void Broker_Constructor_WhenmoduleHandleLowerThanZero_ShouldThrowArgumentOutOfRange()
        {
            ///arrage
            long broker = 0x42;
            long sourceModule = -1;

            ///act
            try
            {
                var brokerInstance = new Broker(broker, sourceModule);
            }
            catch (ArgumentOutOfRangeException e)
            {
                ///assert
                StringAssert.Contains(e.Message, "Invalid source module");
                return;
            }
            Assert.Fail("No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_BROKER_04_002: [ If broker and module are greater than 0, Broker constructor shall save this value and succeed. ] */
        [TestMethod]
        public void Broker_Constructor_succeed()
        {
            ///arrage
            long broker = 0x42;
            long sourceModule = 0x42;

            ///act
            var brokerInstance = new Broker(broker, sourceModule);

            Assert.IsNotNull(brokerInstance);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_BROKER_04_004: [ Publish shall not catch exception thrown by ToByteArray. ] */
        [TestMethod]
        public void Broker_Publish_Does_not_Catch_Exception_by_MessageToByteArray()
        {
            ///arrage
            long broker = 0x42;
            long sourceModule = 0x42;
            var brokerInstance = new Broker(broker, sourceModule);
            Mock<Message> myMessageToPublish = new Mock<Message>(new byte[0], new Dictionary<string, string>());

            myMessageToPublish.Setup(t => t.ToByteArray()).Throws(new FormatException("Fake Exception."));

            ///act
            try
            {
                brokerInstance.Publish(myMessageToPublish.Object);
            }
            catch (FormatException e)
            {
                StringAssert.Contains(e.Message, "Fake Exception.");
                myMessageToPublish.Verify(t => t.ToByteArray());
                return;
            }

            //assert
            Assert.Fail("Publish catch exception by ToByteArray.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_BROKER_04_006: [ If Module_DotNetHost_PublishMessage fails, Publish shall thrown an Application Exception with message saying that Broker Publish failed. ] */
        [TestMethod]
        public void Broker_Publish_throws_when_myDotNetHostWrapper_fail()
        {
            ///arrage
            long broker = 0x42;
            long sourceModule = 0x42;
            Mock<Message> myMessageToPublish = new Mock<Message>(new byte[0], new Dictionary<string, string>());
            Mock<NativeDotNetHostWrapper> myDotNetHostWrapperMock = new Mock<Gateway.NativeDotNetHostWrapper>();
            var brokerInstance = new Broker(broker, sourceModule, myDotNetHostWrapperMock.Object);

            myDotNetHostWrapperMock.Setup(t => t.PublishMessage((IntPtr)broker, (IntPtr)broker, It.IsAny<byte[]>())).Throws(new Exception("Fake Exception."));

            ///act
            try
            {
                brokerInstance.Publish(myMessageToPublish.Object);
            }
            catch(ApplicationException e)
            {
                //assert
                myMessageToPublish.Verify(t => t.ToByteArray());
                myDotNetHostWrapperMock.Verify(t => t.PublishMessage((IntPtr)broker, (IntPtr)broker, It.IsAny<byte[]>()));
                StringAssert.Contains(e.Message, "Failed to Publish Message.");
                return;
            }

            ///cleanup
        }

        /* Tests_SRS_DOTNET_BROKER_04_003: [ Publish shall call the Message.ToByteArray() method to get the Message object translated to byte array. ] */
        /* Tests_SRS_DOTNET_BROKER_04_005: [ Publish shall call the native method Module_DotNetHost_PublishMessage passing the broker and module value saved by it's constructor, the byte[] got from Message and the size of the byte array. ] */
        [TestMethod]
        public void Broker_Publish_succeed()
        {
            ///arrage
            long broker = 0x42;
            long sourceModule = 0x42;
            Mock<Message> myMessageToPublish = new Mock<Message>(new byte[0], new Dictionary<string, string>());
            Mock<NativeDotNetHostWrapper> myDotNetHostWrapperMock = new Mock<NativeDotNetHostWrapper>();
            var brokerInstance = new Broker(broker, sourceModule, myDotNetHostWrapperMock.Object);

            ///act
            brokerInstance.Publish(myMessageToPublish.Object);

            //assert
            myMessageToPublish.Verify(t => t.ToByteArray());
            myDotNetHostWrapperMock.Verify(t => t.PublishMessage((IntPtr)broker, (IntPtr)broker, It.IsAny<byte[]>()));


            ///cleanup
        }
    }
}
