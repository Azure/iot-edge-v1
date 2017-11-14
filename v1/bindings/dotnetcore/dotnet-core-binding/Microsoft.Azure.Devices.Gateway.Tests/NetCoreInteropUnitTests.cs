using System;
using Xunit;
using Microsoft.Azure.Devices.Gateway;
using System.IO;
using Moq;
using System.Collections.Generic;
using System.Reflection;

namespace Microsoft.Azure.Devices.Gateway.Tests
{
    public class NetCoreInteropUnitTests
    {
        static public void anyFakeMethod()
        {

        }

        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_001: [ If any parameter (except configuration) is null, constructor shall throw a ArgumentNullException. ] */
        [Fact]
        public void NetCoreInterop_Create_with_broker_null_arg_throw()
        {
            ///arrage
            Mock<DotNetCoreReflectionLayer> mockedReflectionLayer = new Mock<DotNetCoreReflectionLayer>();
            NetCoreInterop.replaceReflectionLayer(mockedReflectionLayer.Object);

            ///act
            try
            {
                uint returnValue = NetCoreInterop.Create((IntPtr)null, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", "AnyConfiguration");
            }
            catch (ArgumentNullException e)
            {
                ///assert
                ///
                Assert.Contains("broker cannot be null", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }


        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_001: [ If any parameter (except configuration) is null, constructor shall throw a ArgumentNullException. ] */
        [Fact]
        public void NetCoreInterop_Create_with_module_null_arg_throw()
        {
            ///arrage
            Mock<DotNetCoreReflectionLayer> mockedReflectionLayer = new Mock<DotNetCoreReflectionLayer>();
            NetCoreInterop.replaceReflectionLayer(mockedReflectionLayer.Object);

            ///act
            try
            {
                uint returnValue = NetCoreInterop.Create((IntPtr)0x42, (IntPtr)null, "AnyAssemblyName", "AnyEntryType", "AnyConfiguration");
            }
            catch (ArgumentNullException e)
            {
                ///assert
                ///
                Assert.Contains("module cannot be null", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_001: [ If any parameter (except configuration) is null, constructor shall throw a ArgumentNullException. ] */
        [Fact]
        public void NetCoreInterop_Create_with_assemblyName_null_arg_throw()
        {
            ///arrage
            Mock<DotNetCoreReflectionLayer> mockedReflectionLayer = new Mock<DotNetCoreReflectionLayer>();
            NetCoreInterop.replaceReflectionLayer(mockedReflectionLayer.Object);

            ///act
            try
            {
                uint returnValue = NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, null, "AnyEntryType", "AnyConfiguration");
            }
            catch (ArgumentNullException e)
            {
                ///assert
                ///
                Assert.Contains("assemblyName cannot be null", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_001: [ If any parameter (except configuration) is null, constructor shall throw a ArgumentNullException. ] */
        [Fact]
        public void NetCoreInterop_Create_with_entryType_null_arg_throw()
        {
            ///arrage
            Mock<DotNetCoreReflectionLayer> mockedReflectionLayer = new Mock<DotNetCoreReflectionLayer>();
            NetCoreInterop.replaceReflectionLayer(mockedReflectionLayer.Object);

            ///act
            try
            {
                uint returnValue = NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", null, "AnyConfiguration");
            }
            catch (ArgumentNullException e)
            {
                ///assert
                ///
                Assert.Contains("entryType cannot be null", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_001: [ If any parameter (except configuration) is null, constructor shall throw a ArgumentNullException. ] */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_002: [ If the set of modules is not initialized, it shall be. ] */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_003: [ If any underlying api fails, this method shall raise an Exception. ]. */
        [Fact]
        public void NetCoreInterop_Create_with_configuration_null_arg_not_throw()
        {
            ///arrage
            uint returnValue = 0;
            Mock<DotNetCoreReflectionLayer> mockedReflectionLayer = new Mock<DotNetCoreReflectionLayer>();

           
            MethodInfo anyFakeMethod = typeof(NetCoreInteropUnitTests).GetRuntimeMethod("anyFakeMethod", new Type[] {});
                        
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Receive", new Type[] { typeof(Message) })).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Destroy", new Type[] { })).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Start", new Type[] { })).Returns(anyFakeMethod);

            NetCoreInterop.replaceReflectionLayer(mockedReflectionLayer.Object);


            ///act
            returnValue = NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", null);

            /// assert
            Assert.Equal(returnValue.ToString(), "1");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_003: [ If any underlying api fails, this method shall raise an Exception. ]. */
        [Fact]
        public void NetCoreInterop_Create_throws_when_GetType_throws()
        {
            ///arrage
            uint returnValue = 0;
            Mock<DotNetCoreReflectionLayer> mockedReflectionLayer = new Mock<DotNetCoreReflectionLayer>();
            NetCoreInterop.replaceReflectionLayer(mockedReflectionLayer.Object);

            ///act
            try
            {
                returnValue = NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", null);
            }
            catch (Exception)
            {
                ///assert
                ///
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_003: [ If any underlying api fails, this method shall raise an Exception. ]. */
        [Fact]
        public void NetCoreInterop_Create_throws_when_CreateInstance_throws()
        {
            ///arrage
            uint returnValue = 0;

            Mock<DotNetCoreReflectionLayer> mockedReflectionLayer = new Mock<DotNetCoreReflectionLayer>();

            mockedReflectionLayer.Setup(t => t.CreateInstance(null)).Throws(new Exception("Fake Exception."));

            NetCoreInterop.replaceReflectionLayer(mockedReflectionLayer.Object);

            ///act
            try
            {
                returnValue = NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", null);
            }
            catch (Exception)
            {
                ///assert
                ///
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }


        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_003: [ If any underlying api fails, this method shall raise an Exception. ]. */
        [Fact]
        public void NetCoreInterop_Create_throws_when_GetMethod_throws()
        {
            ///arrage
            uint returnValue = 0;

            Mock<DotNetCoreReflectionLayer> mockedReflectionLayer = new Mock<DotNetCoreReflectionLayer>();

            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Create", null)).Throws(new Exception("Fake Exception."));

            NetCoreInterop.replaceReflectionLayer(mockedReflectionLayer.Object);

            ///act
            try
            {
                returnValue = NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", null);
            }
            catch (Exception)
            {
                ///assert
                ///
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_003: [ If any underlying api fails, this method shall raise an Exception. ]. */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_009: [ Create shall call Create method on client module. ] */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_010: [ If Create module on client module succeeds return moduleID, otherwise raise an Exception ] */
        [Fact]
        public void NetCoreInterop_Create_throws_when_InvokeMethod_throws()
        {
            ///arrage
            uint returnValue = 0;

            Mock<DotNetCoreReflectionLayer> mockedReflectionLayer = new Mock<DotNetCoreReflectionLayer>();
            mockedReflectionLayer.Setup(t => t.InvokeMethod(null, null, null)).Throws(new Exception("Fake Exception."));
            NetCoreInterop.replaceReflectionLayer(mockedReflectionLayer.Object);

            ///act
            try
            {
                returnValue = NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", "AnyConfiguration");
            }
            catch (Exception)
            {
                ///assert
                ///
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_003: [ If any underlying api fails, this method shall raise an Exception. ]. */
        [Fact]
        public void NetCoreInterop_Create_throws_when_GetMethod_ForReceive_throws()
        {
            ///arrage
            uint returnValue = 0;

            Mock<DotNetCoreReflectionLayer> mockedReflectionLayer = new Mock<DotNetCoreReflectionLayer>();
            MethodInfo anyFakeMethod = typeof(NetCoreInteropUnitTests).GetRuntimeMethod("anyFakeMethod", new Type[] { });

            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Receive", new Type[] { typeof(Message) })).Throws(new Exception("Receive Exception."));
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Destroy", null)).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Start", null)).Returns(anyFakeMethod);
            NetCoreInterop.replaceReflectionLayer(mockedReflectionLayer.Object);

            ///act
            try
            {
                returnValue = NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", "AnyConfiguration");
            }
            catch (Exception e)
            {
                ///assert
                ///
                Assert.Contains("Receive Exception.", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_003: [ If any underlying api fails, this method shall raise an Exception. ]. */
        [Fact]
        public void NetCoreInterop_Create_throws_when_GetMethod_ForDestroy_throws()
        {
            ///arrage
            uint returnValue = 0;

            Mock<DotNetCoreReflectionLayer> mockedReflectionLayer = new Mock<DotNetCoreReflectionLayer>();
            MethodInfo anyFakeMethod = typeof(NetCoreInteropUnitTests).GetRuntimeMethod("anyFakeMethod", new Type[] { });

            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Receive", new Type[] { typeof(Message) })).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Destroy", new Type[] { })).Throws(new Exception("Destroy Exception."));
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Start", new Type[] { })).Returns(anyFakeMethod);
            NetCoreInterop.replaceReflectionLayer(mockedReflectionLayer.Object);

            ///act
            try
            {
                returnValue = NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", "AnyConfiguration");
            }
            catch (Exception e)
            {
                ///assert
                ///
                Assert.Contains("Destroy Exception.", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_003: [ If any underlying api fails, this method shall raise an Exception. ]. */
        [Fact]
        public void NetCoreInterop_Create_throws_when_GetMethod_ForStart_throws()
        {
            ///arrage
            uint returnValue = 0;

            Mock<DotNetCoreReflectionLayer> mockedReflectionLayer = new Mock<DotNetCoreReflectionLayer>();
            MethodInfo anyFakeMethod = typeof(NetCoreInteropUnitTests).GetRuntimeMethod("anyFakeMethod", new Type[] { });

            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Receive", new Type[] { typeof(Message) })).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Destroy", new Type[] { })).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Start", new Type[] { })).Throws(new Exception("Start Exception."));
            NetCoreInterop.replaceReflectionLayer(mockedReflectionLayer.Object);

            ///act
            try
            {
                returnValue = NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", "AnyConfiguration");
            }
            catch (Exception e)
            {
                ///assert
                ///
                Assert.Contains("Start Exception.", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }


        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_003: [ If any underlying api fails, this method shall raise an Exception. ]. */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_008: [ If client .NET Core module doesn't implement one of the required methods Create, Receive or Destroy, Create shall raise an exception. ] */
        [Fact]
        public void NetCoreInterop_Create_throws_when_Receive_is_Missing_throws()
        {
            ///arrage
            uint returnValue = 0;

            Mock<DotNetCoreReflectionLayer> mockedReflectionLayer = new Mock<DotNetCoreReflectionLayer>();
            MethodInfo anyFakeMethod = typeof(NetCoreInteropUnitTests).GetRuntimeMethod("anyFakeMethod", new Type[] { });

            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Destroy", null)).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Start", null)).Returns(anyFakeMethod);
            NetCoreInterop.replaceReflectionLayer(mockedReflectionLayer.Object);

            ///act
            try
            {
                returnValue = NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", "AnyConfiguration");
            }
            catch (Exception e)
            {
                ///assert
                ///
                Assert.Contains("Missing implementation of Create, Destroy or Receive.", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_003: [ If any underlying api fails, this method shall raise an Exception. ]. */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_008: [ If client .NET Core module doesn't implement one of the required methods Create, Receive or Destroy, Create shall raise an exception. ] */
        [Fact]
        public void NetCoreInterop_Create_throws_when_Destroy_is_Missing_throws()
        {
            ///arrage
            uint returnValue = 0;

            Mock<DotNetCoreReflectionLayer> mockedReflectionLayer = new Mock<DotNetCoreReflectionLayer>();
            MethodInfo anyFakeMethod = typeof(NetCoreInteropUnitTests).GetRuntimeMethod("anyFakeMethod", new Type[] { });

            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Receive", null)).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Start", null)).Returns(anyFakeMethod);
            NetCoreInterop.replaceReflectionLayer(mockedReflectionLayer.Object);

            ///act
            try
            {
                returnValue = NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", "AnyConfiguration");
            }
            catch (Exception e)
            {
                ///assert
                ///
                Assert.Contains("Missing implementation of Create, Destroy or Receive.", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_003: [ If any underlying api fails, this method shall raise an Exception. ]. */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_008: [ If client .NET Core module doesn't implement one of the required methods Create, Receive or Destroy, Create shall raise an exception. ] */
        [Fact]
        public void NetCoreInterop_Create_throws_when_Start_is_Missing_throws()
        {
            ///arrage
            uint returnValue = 0;

            Mock<DotNetCoreReflectionLayer> mockedReflectionLayer = new Mock<DotNetCoreReflectionLayer>();
            MethodInfo anyFakeMethod = typeof(NetCoreInteropUnitTests).GetRuntimeMethod("anyFakeMethod", new Type[] { });

            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Receive", null)).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Destroy", null)).Returns(anyFakeMethod);
            NetCoreInterop.replaceReflectionLayer(mockedReflectionLayer.Object);

            ///act
            try
            {
                returnValue = NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", "AnyConfiguration");
            }
            catch (Exception e)
            {
                ///assert
                ///
                Assert.Contains("Missing implementation of Create, Destroy or Receive.", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }


        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_004: [ Create shall create an instance of DotNetCoreModuleInstance to store module information; ] */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_005: [ Create shall load client library identified by assemblyName. ] */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_006: [ Create shall get types from loaded assembly and check if .NET Core module has implemented IGatewayModule. ] */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_007: [ Create shall save delegates to Receive, Destroy and Start(if implemented) to DotNetCoreModuleInstance. ] */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_010: [ If Create module on client module succeeds return moduleID, otherwise raise an Exception ] */
        [Fact]
        public void NetCoreInterop_create_a_module_success()
        {
            ///arrage
            uint returnValue = 0;
            Mock<DotNetCoreReflectionLayer> mockedReflectionLayer = new Mock<DotNetCoreReflectionLayer>();


            MethodInfo anyFakeMethod = typeof(NetCoreInteropUnitTests).GetRuntimeMethod("anyFakeMethod", new Type[] { });

            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Receive", new Type[] { typeof(Message) })).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Destroy", new Type[] { })).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Start", new Type[] { })).Returns(anyFakeMethod);

            NetCoreInterop.replaceReflectionLayer(mockedReflectionLayer.Object);

            ///act
            returnValue = NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", "AnyConfiguration");

            /// assert
            Assert.Equal(returnValue.ToString(), "1");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_011: [ Receive shall get the DotNetCoreModuleInstance based on moduleID ] */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_019: [ Receive shall raise an Exception if module can not be found based on moduleID ] */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_021: [ Receive shall raise an Exception if module can't be found. ] */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_009: [ Create shall call Create method on client module. ] */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_024: [Create shall verify if the module contains the type describe by entryType and check of methods Create, Destroy, Receive and Start were implemented on that type. ]*/
        [Fact]
        public void NetCoreInterop_Receive_with_moduleid_that_not_exists_throw()
        {
            ///arrage
            Mock<DotNetCoreReflectionLayer> mockedReflectionLayer = new Mock<DotNetCoreReflectionLayer>();
            MethodInfo anyFakeMethod = typeof(NetCoreInteropUnitTests).GetRuntimeMethod("anyFakeMethod", new Type[] { });


            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Receive", new Type[] { typeof(Message) })).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Destroy", new Type[] { })).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Start", new Type[] { })).Returns(anyFakeMethod);
            NetCoreInterop.replaceReflectionLayer(mockedReflectionLayer.Object);

            //Make sure we create the dictioary.
            NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", "AnyConfiguration");

            byte[] notFail____minimalMessage =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 14,   /*size of this array*/
                0x00, 0x00, 0x00, 0x00, /*zero properties*/
                0x00, 0x00, 0x00, 0x00  /*zero message content size*/
            };

            ///act
            var messageInstance = new Message(notFail____minimalMessage);
            uint messageSize = uint.Parse(messageInstance.Content.GetLength(0).ToString()) + 14;

            ///act
            try
            {
                NetCoreInterop.Receive(notFail____minimalMessage, messageSize, 42);
            }
            catch (Exception e)
            {
                ///assert
                ///
                Assert.Contains("Module 42 can't be found.", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_011: [ Receive shall get the DotNetCoreModuleInstance based on moduleID ] */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_019: [ Receive shall raise an Exception if module can not be found based on moduleID ] */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_021: [ Receive shall raise an Exception if module can't be found. ] */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_009: [ Create shall call Create method on client module. ] */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_024: [Create shall verify if the module contains the type describe by entryType and check of methods Create, Destroy, Receive and Start were implemented on that type. ]*/
        [Fact]
        public void NetCoreInterop_Receive_with_moduleid_that_exists_succeed()
        {
            ///arrage
            Mock<DotNetCoreReflectionLayer> mockedReflectionLayer = new Mock<DotNetCoreReflectionLayer>();

            MethodInfo anyFakeMethod = typeof(NetCoreInteropUnitTests).GetRuntimeMethod("anyFakeMethod", new Type[] { });
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Receive", new Type[] { typeof(Message) })).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Destroy", new Type[] { })).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Start", new Type[] { })).Returns(anyFakeMethod);

            mockedReflectionLayer.Setup(t => t.InvokeMethod(null, anyFakeMethod, null));


            NetCoreInterop.replaceReflectionLayer(mockedReflectionLayer.Object);
            uint moduleCreated = 0;
            //Make sure we create the dictioary.
            try
            {
                moduleCreated = NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", "AnyConfiguration");
            }
            catch (FileNotFoundException)
            {
                ///assert
                ///
            }

            byte[] notFail____minimalMessage =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 14,   /*size of this array*/
                0x00, 0x00, 0x00, 0x00, /*zero properties*/
                0x00, 0x00, 0x00, 0x00  /*zero message content size*/
            };

            ///act
            Message messageInstance = new Message(notFail____minimalMessage);
            uint messageSize = uint.Parse(messageInstance.Content.GetLength(0).ToString()) + 14;

            ///act
            NetCoreInterop.Receive(notFail____minimalMessage, messageSize, moduleCreated);

            ///assert
            mockedReflectionLayer.Verify(t => t.InvokeMethod(null, anyFakeMethod, It.IsAny<Object[]>()));


            ///cleanup
        }


        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_014: [ Destroy shall get the DotNetCoreModuleInstance based on moduleID ] */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_022: [ Destroy shall raise an Exception if module can't be found. ] */
        [Fact]
        public void NetCoreInterop_Destroy_with_moduleid_that_not_exists_throw()
        {
            ///arrage
            Mock<DotNetCoreReflectionLayer> mockedReflectionLayer = new Mock<DotNetCoreReflectionLayer>();
            MethodInfo anyFakeMethod = typeof(NetCoreInteropUnitTests).GetRuntimeMethod("anyFakeMethod", new Type[] { });

            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Receive", new Type[] { typeof(Message) })).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Destroy", new Type[] { })).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Start", new Type[] { })).Returns(anyFakeMethod);

            NetCoreInterop.replaceReflectionLayer(mockedReflectionLayer.Object);


            //Make sure we create the dictioary.
            NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", "AnyConfiguration");

            ///act
            try
            {
                NetCoreInterop.Destroy(42);
            }
            catch (Exception e)
            {
                ///assert
                ///
                Assert.Contains("Module 42 can't be found.", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }


        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_014: [ Destroy shall get the DotNetCoreModuleInstance based on moduleID ] */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_015: [ Destroy shall invoke .NET Core client method Destroy. ] */
        [Fact]
        public void NetCoreInterop_Destroy_with_moduleid_that_exists_succeed()
        {
            ///arrage
            Mock<DotNetCoreReflectionLayer> mockedReflectionLayer = new Mock<DotNetCoreReflectionLayer>();

            MethodInfo anyFakeMethod = typeof(NetCoreInteropUnitTests).GetRuntimeMethod("anyFakeMethod", new Type[] { });
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Receive", new Type[] { typeof(Message) })).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Destroy", new Type[] { })).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Start", new Type[] { })).Returns(anyFakeMethod);

            mockedReflectionLayer.Setup(t => t.InvokeMethod(null, anyFakeMethod, null));


            NetCoreInterop.replaceReflectionLayer(mockedReflectionLayer.Object);
            uint moduleCreated = 0;
            //Make sure we create the dictioary.

            moduleCreated = NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", "AnyConfiguration");
            
            ///act
            NetCoreInterop.Destroy(moduleCreated);

            ///assert
            mockedReflectionLayer.Verify(t => t.InvokeMethod(null, anyFakeMethod, It.IsAny<Object[]>()));


            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_020: [ Destroy shall remove module from the set of modules. ] */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_022: [ Destroy shall raise an Exception if module can't be found. ] */
        [Fact]
        public void NetCoreInterop_Destroy_with_moduleid_that_exists_2_times_throws()
        {
            ///arrage
            Mock<DotNetCoreReflectionLayer> mockedReflectionLayer = new Mock<DotNetCoreReflectionLayer>();

            MethodInfo anyFakeMethod = typeof(NetCoreInteropUnitTests).GetRuntimeMethod("anyFakeMethod", new Type[] { });
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Receive", new Type[] { typeof(Message) })).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Destroy", new Type[] { })).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Start", new Type[] { })).Returns(anyFakeMethod);

            mockedReflectionLayer.Setup(t => t.InvokeMethod(null, anyFakeMethod, null));


            NetCoreInterop.replaceReflectionLayer(mockedReflectionLayer.Object);
            uint moduleCreated = 0;
            //Make sure we create the dictioary.

            moduleCreated = NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", "AnyConfiguration");

            ///act
            ///assert
            NetCoreInterop.Destroy(moduleCreated);
            mockedReflectionLayer.Verify(t => t.InvokeMethod(null, anyFakeMethod, It.IsAny<Object[]>()));
            try
            {
                NetCoreInterop.Destroy(moduleCreated);
            }
            catch(Exception e)
            {
                ///assert
                ///
                Assert.Contains("Module 1 can't be found.", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");


            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_016: [ Start shall get the DotNetCoreModuleInstance based on moduleID ] */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_023: [ Start shall raise an Exception if module can't be found. ] */
        [Fact]
        public void NetCoreInterop_Start_with_moduleid_that_not_exists_throw()
        {
            ///arrage
            Mock<DotNetCoreReflectionLayer> mockedReflectionLayer = new Mock<DotNetCoreReflectionLayer>();
            MethodInfo anyFakeMethod = typeof(NetCoreInteropUnitTests).GetRuntimeMethod("anyFakeMethod", new Type[] { });
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Receive", new Type[] { typeof(Message) })).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Destroy", new Type[] { })).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Start", new Type[] { })).Returns(anyFakeMethod);

            NetCoreInterop.replaceReflectionLayer(mockedReflectionLayer.Object);


            //Make sure we create the dictioary.
            NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", "AnyConfiguration");

            ///act
            try
            {
                NetCoreInterop.Start(42);
            }
            catch (Exception e)
            {
                ///assert
                ///
                Assert.Contains("Module 42 can't be found.", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_018: [ Start shall invoke .NET Core client method Start. ] */
        [Fact]
        public void NetCoreInterop_Start_with_moduleid_that_exists_succeed()
        {
            ///arrage
            Mock<DotNetCoreReflectionLayer> mockedReflectionLayer = new Mock<DotNetCoreReflectionLayer>();

            MethodInfo anyFakeMethod = typeof(NetCoreInteropUnitTests).GetRuntimeMethod("anyFakeMethod", new Type[] { });
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Receive", new Type[] { typeof(Message) })).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Destroy", new Type[] { })).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Start", new Type[] { })).Returns(anyFakeMethod);

            mockedReflectionLayer.Setup(t => t.InvokeMethod(null, anyFakeMethod, null));


            NetCoreInterop.replaceReflectionLayer(mockedReflectionLayer.Object);
            uint moduleCreated = 0;
            //Make sure we create the dictioary.

            moduleCreated = NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", "AnyConfiguration");

            ///act
            NetCoreInterop.Start(moduleCreated);

            ///assert
            mockedReflectionLayer.Verify(t => t.InvokeMethod(null, anyFakeMethod, It.IsAny<Object[]>()));


            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_017: [ If .NET Core client module did not implement start, Start shall do nothing. ] */
        [Fact]
        public void NetCoreInterop_Start_module_withoutStart_do_nothing()
        {
            ///arrage
            Mock<DotNetCoreReflectionLayer> mockedReflectionLayer = new Mock<DotNetCoreReflectionLayer>();

            MethodInfo anyFakeMethod = typeof(NetCoreInteropUnitTests).GetRuntimeMethod("anyFakeMethod", new Type[] { });
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Receive", new Type[] { typeof(Message) })).Returns(anyFakeMethod);
            mockedReflectionLayer.Setup(t => t.GetMethod(null, "Destroy", new Type[] { })).Returns(anyFakeMethod);

            mockedReflectionLayer.Setup(t => t.InvokeMethod(null, anyFakeMethod, null));


            NetCoreInterop.replaceReflectionLayer(mockedReflectionLayer.Object);
            uint moduleCreated = 0;
            //Make sure we create the dictioary.

            moduleCreated = NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", "AnyConfiguration");

            ///act
            NetCoreInterop.Start(moduleCreated);

            ///assert
            ///cleanup
        }
    }
}
