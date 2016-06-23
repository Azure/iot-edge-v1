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
    public class MessageBusUnitTest
    {
        /* Tests_SRS_DOTNET_MESSAGEBUS_04_001: [ If msgBus is <= 0, MessageBus constructor shall throw a new ArgumentException ] */
        [TestMethod]
        public void MessageBus_Constructor_WhenMsgBusLowerThanZero_ShouldThrowArgumentOutOfRange()
        {
            ///arrage
            long invalidMsgBus = -1;
            long validSourceModule = 0x42;

            ///act
            try
            {
                var messageBusInstance = new MessageBus(invalidMsgBus, validSourceModule);
            }
            catch (ArgumentOutOfRangeException e)
            {
                ///assert
                StringAssert.Contains(e.Message, "Invalid Msg Bus.");
                return;
            }
            Assert.Fail("No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGEBUS_04_007: [ If moduleHandle is <= 0, MessageBus constructor shall throw a new ArgumentException ] */
        [TestMethod]
        public void MessageBus_Constructor_WhenmoduleHandleLowerThanZero_ShouldThrowArgumentOutOfRange()
        {
            ///arrage
            long msgBus = 0x42;
            long sourceModule = -1;

            ///act
            try
            {
                var messageBusInstance = new MessageBus(msgBus, sourceModule);
            }
            catch (ArgumentOutOfRangeException e)
            {
                ///assert
                StringAssert.Contains(e.Message, "Invalid source Module Handle.");
                return;
            }
            Assert.Fail("No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGEBUS_04_002: [ If msgBus and moduleHandle is greater than 0, MessageBus constructor shall save this value and succeed. ] */
        [TestMethod]
        public void MessageBus_Constructor_succeed()
        {
            ///arrage
            long msgBus = 0x42;
            long sourceModule = 0x42;

            ///act
            var messageBusInstance = new MessageBus(msgBus, sourceModule);

            Assert.IsNotNull(messageBusInstance);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGEBUS_04_004: [ Publish shall not catch exception thrown by ToByteArray. ] */
        [TestMethod]
        public void MessageBus_Publish_Does_not_Catch_Exception_by_MessageToByteArray()
        {
            ///arrage
            long msgBus = 0x42;
            long sourceModule = 0x42;
            var messageBusInstance = new MessageBus(msgBus, sourceModule);
            Mock<Message> myMessageToPublish = new Mock<Message>(new byte[0], new Dictionary<string, string>());

            myMessageToPublish.Setup(t => t.ToByteArray()).Throws(new FormatException("Fake Exception."));

            ///act
            try
            {
                messageBusInstance.Publish(myMessageToPublish.Object);
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

        /* Tests_SRS_DOTNET_MESSAGEBUS_04_006: [ If Module_DotNetHost_PublishMessage fails, Publish shall thrown an Application Exception with message saying that MessageBus Publish failed. ] */
        [TestMethod]
        public void MessageBus_Publish_throws_when_myDotNetHostWrapper_fail()
        {
            ///arrage
            long msgBus = 0x42;
            long sourceModule = 0x42;
            Mock<Message> myMessageToPublish = new Mock<Message>(new byte[0], new Dictionary<string, string>());
            Mock<NativeDotNetHostWrapper> myDotNetHostWrapperMock = new Mock<Gateway.NativeDotNetHostWrapper>();
            var messageBusInstance = new MessageBus(msgBus, sourceModule, myDotNetHostWrapperMock.Object);

            myDotNetHostWrapperMock.Setup(t => t.PublishMessage((IntPtr)msgBus, (IntPtr)msgBus, It.IsAny<byte[]>())).Throws(new Exception("Fake Exception."));

            ///act
            try
            {
                messageBusInstance.Publish(myMessageToPublish.Object);
            }
            catch(ApplicationException e)
            {
                //assert
                myMessageToPublish.Verify(t => t.ToByteArray());
                myDotNetHostWrapperMock.Verify(t => t.PublishMessage((IntPtr)msgBus, (IntPtr)msgBus, It.IsAny<byte[]>()));
                StringAssert.Contains(e.Message, "Failed to Publish Message.");
                return;
            }

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGEBUS_04_003: [ Publish shall call the Message.ToByteArray() method to get the Message object translated to byte array. ] */
        /* Tests_SRS_DOTNET_MESSAGEBUS_04_005: [ Publish shall call the native method Module_DotNetHost_PublishMessage passing the msgBus and moduleHandle value saved by it's constructor, the byte[] got from Message and the size of the byte array. ] */
        [TestMethod]
        public void MessageBus_Publish_succeed()
        {
            ///arrage
            long msgBus = 0x42;
            long sourceModule = 0x42;
            Mock<Message> myMessageToPublish = new Mock<Message>(new byte[0], new Dictionary<string, string>());
            Mock<NativeDotNetHostWrapper> myDotNetHostWrapperMock = new Mock<NativeDotNetHostWrapper>();
            var messageBusInstance = new MessageBus(msgBus, sourceModule, myDotNetHostWrapperMock.Object);

            ///act
            messageBusInstance.Publish(myMessageToPublish.Object);

            //assert
            myMessageToPublish.Verify(t => t.ToByteArray());
            myDotNetHostWrapperMock.Verify(t => t.PublishMessage((IntPtr)msgBus, (IntPtr)msgBus, It.IsAny<byte[]>()));


            ///cleanup
        }
    }
}
