using System;
using Microsoft.VisualStudio.TestPlatform.UnitTestFramework;
using System.Collections.Generic;
using Microsoft.Azure.IoT.Gateway;
using System.Text;

namespace Microsoft.Azure.IoT.Gateway.Test
{
    [TestClass]
    public class TestGateway
    {
        [TestMethod]
        public void TestEmptyGateway()
        {
            {
                var configProps = new Dictionary<string, string>();
                var modules = new List<IGatewayModule>();
                var gateway = new Gateway(modules, configProps);
            }
            Assert.IsTrue(true);
        }

        [TestMethod]
        public void TestInvalidArguments()
        {
            var props = new Dictionary<string, string>();
            var modules = new List<IGatewayModule>();

            try { new Gateway(null, props); Assert.IsTrue(false); } catch (ArgumentException e) { Assert.IsTrue(true); }
            try { new Gateway(modules, null); Assert.IsTrue(false); } catch (ArgumentException e) { Assert.IsTrue(true); }

        }

        private class TestModule : IGatewayModule
        {
            public IReadOnlyDictionary<string, string> config;
            public MessageBus bus;
            public Message msg;
            public int create, receive;

            public TestModule()
            {
                config = null;
                bus = null;
                msg = null;
                create = 0;
                receive = 0;
            }

            ~TestModule()
            {

            }

            public void Create(MessageBus bus, IReadOnlyDictionary<string, string> configuration)
            {
                this.create++;
                this.bus = bus;
                this.config = configuration;
            }

            public void Destroy()
            {
            }

            public void Receive(Message received_message)
            {
                this.receive++;
                this.msg = received_message;
            }
        }

        private void ValidateTestModuleState(
            TestModule testModule, 
            int create, int receive, 
            IReadOnlyDictionary<string, string> configProps, 
            IList<byte> msgContent, IReadOnlyDictionary<string, string> msgProps)
        {
            Assert.AreEqual(create, testModule.create);
            Assert.AreEqual(receive, testModule.receive);

            if (create > 0)
            {
                Assert.AreEqual(configProps, testModule.config);
                Assert.IsNotNull(testModule.bus);
            }

            if (receive > 0)
            {
                Assert.IsNotNull(testModule.msg);
                var testModuleContent = testModule.msg.GetContent();
                Assert.AreEqual(msgContent.Count, testModuleContent.Count);
                for (int i = 0; i < msgContent.Count; i++)
                {
                    Assert.AreEqual(msgContent[i], testModuleContent[i]);
                }
                var testModuleProps = testModule.msg.GetProperties();
                Assert.AreEqual(msgProps.Count, testModuleProps.Count);
                foreach (var testModulePropKey in testModuleProps.Keys)
                {
                    Assert.IsTrue(msgProps.ContainsKey(testModulePropKey));
                    Assert.AreEqual(msgProps[testModulePropKey], testModuleProps[testModulePropKey]);
                }
            }
        }

        [TestMethod]
        public void TestSimpleGateway()
        {
            TestModule testModule1 = new TestModule();
            TestModule testModule2 = new TestModule();

            var configProps = new Dictionary<string, string>();
            configProps.Add("a", "b");
            var modules = new List<IGatewayModule>();
            modules.Add(testModule1);
            modules.Add(testModule2);
            Gateway gateway = new Gateway(modules, configProps);

            ValidateTestModuleState(testModule1, 1, 0, configProps, null, null);
            ValidateTestModuleState(testModule2, 1, 0, configProps, null, null);

            var msgProps = new Dictionary<string, string>();
            msgProps.Add("high", "low");
            var msg = new Message("hi", msgProps);

            testModule1.bus.Publish(msg);

            // ensure that testModule1 doesn't see its own message
            ValidateTestModuleState(testModule1, 1, 0, configProps, null, null);
            // ensure that testModule2 sees testModule1's message
            ValidateTestModuleState(testModule2, 1, 1, configProps, msg.GetContent(), msg.GetProperties());

        }

    }
}
