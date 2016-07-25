using System;
using Microsoft.VisualStudio.TestPlatform.UnitTestFramework;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.Azure.IoT.Gateway.Test
{
    [TestClass]
    public class TestMessage
    {
        [TestMethod]
        public void TestMessageByteArrayConstructor()
        {
            // borrowed from azure-iot-gateway-sdk\bindings\dotnet\dotnet-binding\Microsoft.Azure.IoT.Gateway.Test\MessageUnitTests.cs
            byte[] notFail__2Property_2bytes =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 64,   /*size of this array*/
                0x00, 0x00, 0x00, 0x02, /*two properties*/
                (byte)'A', (byte)'z',(byte)'u',(byte)'r',(byte)'e',(byte)' ',(byte)'I',(byte)'o',(byte)'T',(byte)' ',(byte)'G',(byte)'a',(byte)'t',(byte)'e',(byte)'w',(byte)'a',(byte)'y',(byte)' ',(byte)'i',(byte)'s',(byte)'\0',(byte)'a',(byte)'w',(byte)'e',(byte)'s',(byte)'o',(byte)'m',(byte)'e',(byte)'\0',
                (byte)'B',(byte)'l',(byte)'e',(byte)'e',(byte)'d',(byte)'i',(byte)'n',(byte)'g',(byte)'E',(byte)'d',(byte)'g',(byte)'e',(byte)'\0',(byte)'r',(byte)'o',(byte)'c',(byte)'k',(byte)'s',(byte)'\0',
                0x00, 0x00, 0x00, 0x02,  /*1 message content size*/
                (byte)'3',(byte)'4'
            };

            var msg = new Message(new List<byte>(notFail__2Property_2bytes));
            var msgContent = msg.GetContent();
            var msgProps = msg.GetProperties();

            int index = 2 + 4 + 3;
            int indexFirstProp = index + 1;

            // Test Props
            Assert.AreEqual(notFail__2Property_2bytes[index++], msgProps.Count);
            foreach (var key in msgProps.Keys)
            {
                for (int i = 0; i < key.Length; i++)
                {
                    Assert.AreEqual((byte)notFail__2Property_2bytes[index++], (byte)key[i]);
                }

                index++;

                var value = msgProps[key];
                for (int i = 0; i < value.Length; i++)
                {
                    Assert.AreEqual((byte)notFail__2Property_2bytes[index++], (byte)value[i]);
                }

                index++;
            }

            index += 3;

            // Test Content
            Assert.AreEqual(notFail__2Property_2bytes[index++], msgContent.Count);
            for (int i = 0; i < msgContent.Count; i++)
            {
                Assert.AreEqual((byte)notFail__2Property_2bytes[index++], (byte)msgContent[i]);
            }

            var roundTripBytes = msg.ToBytes();
            Assert.AreEqual(notFail__2Property_2bytes.Length, roundTripBytes.Count);
            for (int i = 0; i < roundTripBytes.Count; i++)
            {
                Assert.AreEqual(notFail__2Property_2bytes[i], roundTripBytes[i]);
            }

        }

        private void ValidateMessage(
            IList<byte> expectedContent, IReadOnlyDictionary<string, string> expectedProps,
            IList<byte> actualContent, IReadOnlyDictionary<string, string> actualProps)
        {
            Assert.AreEqual(expectedContent.Count, actualContent.Count);
            for (int i = 0; i < expectedContent.Count; i++)
            {
                Assert.AreEqual(expectedContent[i], actualContent[i]);
            }

            Assert.AreEqual(expectedProps.Count, actualProps.Count);
            foreach (var msgPropKey in actualProps.Keys)
            {
                Assert.IsTrue(expectedProps.ContainsKey(msgPropKey));
                Assert.AreEqual(expectedProps[msgPropKey], actualProps[msgPropKey]);
            }
        }

        [TestMethod]
        public void TestMessageModernConstructor()
        {
            var content = "foo";
            var props = new Dictionary<string, string>();
            props.Add("blah", "gah");
            props.Add("clam", "chowder");
            props.Add("bacon", "bits");
            props.Add("durango", "Colorado");
            var msg = new Message(content, props);
            var msgContent = msg.GetContent();
            var msgProps = msg.GetProperties();

            var contentAsBytes = new List<byte>(Encoding.UTF8.GetBytes(content));
            ValidateMessage(contentAsBytes, props, msgContent, msgProps);
        }

        [TestMethod]
        public void TestMessageModernConstructor_NoContent()
        {
            var content = "";
            var props = new Dictionary<string, string>();
            props.Add("blah", "gah");
            props.Add("clam", "chowder");
            props.Add("bacon", "bits");
            props.Add("durango", "Colorado");
            var msg = new Message(content, props);
            var msgContent = msg.GetContent();
            var msgProps = msg.GetProperties();

            var contentAsBytes = new List<byte>(Encoding.UTF8.GetBytes(content));
            ValidateMessage(contentAsBytes, props, msgContent, msgProps);
        }

        [TestMethod]
        public void TestMessageModernConstructor_NoProps()
        {
            var content = "hats";
            var props = new Dictionary<string, string>();
            var msg = new Message(content, props);
            var msgContent = msg.GetContent();
            var msgProps = msg.GetProperties();

            var contentAsBytes = new List<byte>(Encoding.UTF8.GetBytes(content));
            ValidateMessage(contentAsBytes, props, msgContent, msgProps);
        }

        [TestMethod]
        public void TestMessageModernConstructor_Empty()
        {
            var content = "";
            var props = new Dictionary<string, string>();
            var msg = new Message(content, props);
            var msgContent = msg.GetContent();
            var msgProps = msg.GetProperties();

            var contentAsBytes = new List<byte>(Encoding.UTF8.GetBytes(content));
            ValidateMessage(contentAsBytes, props, msgContent, msgProps);
        }

        [TestMethod]
        public void TestMessageByteArrayConstructors()
        {
            var content = new List<byte>();
            for (int i = 1; i < 100; i++) { content.Add((byte)i); }
            var props = new Dictionary<string, string>();
            props.Add("blah", "gah");
            props.Add("clam", "chowder");
            props.Add("bacon", "bits");
            props.Add("durango", "Colorado");
            var msg = Message.CreateMessage(content, props);
            var msgContent = msg.GetContent();
            var msgProps = msg.GetProperties();

            ValidateMessage(content, props, msgContent, msgProps);
        }

        [TestMethod]
        public void TestMessageChinese()
        {
            var content = "辉煌的混蛋";
            var props = new Dictionary<string, string>();
            props.Add("辉煌的混蛋", "辉煌的混蛋");
            props.Add("辉煌的混", "辉煌的混");

            var msg = new Message(content, props);
            var msgContent = msg.GetContent();
            var msgProps = msg.GetProperties();

            var contentAsBytes = new List<byte>(Encoding.UTF8.GetBytes(content));
            ValidateMessage(contentAsBytes, props, msgContent, msgProps);
        }

        [TestMethod]
        public void TestCopyConstructor()
        {
            var content = "辉煌的混蛋";
            var props = new Dictionary<string, string>();
            props.Add("辉煌的混蛋", "辉煌的混蛋");
            props.Add("辉煌的混", "辉煌的混");

            var msg1 = new Message(content, props);
            var msg1Content = msg1.GetContent();
            var msg1Props = msg1.GetProperties();

            var msg2 = Message.CreateMessage(msg1);
            var msg2Content = msg2.GetContent();
            var msg2Props = msg2.GetProperties();

            ValidateMessage(msg1Content, msg1Props, msg2Content, msg2Props);
        }

        [TestMethod]
        public void TestInvalidArguments()
        {
            var content = String.Empty;
            var props = new Dictionary<string, string>();
            var bytes = new List<byte>();

            try { new Message(null); Assert.IsTrue(false); } catch(ArgumentException e) { Assert.IsTrue(true); }
            try { new Message(null, props); Assert.IsTrue(false); } catch (ArgumentException e) { Assert.IsTrue(true); }
            try { new Message(content, null); Assert.IsTrue(false); } catch (ArgumentException e) { Assert.IsTrue(true); }
            try { Message.CreateMessage(null); Assert.IsTrue(false); } catch (ArgumentException e) { Assert.IsTrue(true); }
            try { Message.CreateMessage(null, props); Assert.IsTrue(false); } catch (ArgumentException e) { Assert.IsTrue(true); }
            try { Message.CreateMessage(bytes, null); Assert.IsTrue(false); } catch (ArgumentException e) { Assert.IsTrue(true); }

        }
    }
}
