using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.Azure.IoT.Gateway;
using Moq;

namespace Microsoft.Azure.IoT.Gateway.Test
{

    [TestClass]
    public class MessageUnitTests
    {
        /* Tests_SRS_DOTNET_MESSAGE_04_001: [ Message class shall have an empty constructor which will create a message with an empty Content and empry Properties ] */
        [TestMethod]
        public void Message_Constructor_Empty()
        {
            ///arrage

            ///act
            var messageInstance = new Message();

            var test = messageInstance.ToByteArray();


            ///cleanup
        }

        [TestMethod]
        public void Message_NULL_reference()
        {
            ///arrage

            ///act
            var messageInstance = new Message((byte[])null, null);

            var test = messageInstance.ToByteArray();


            ///cleanup
        }


    }
}
