using System;
using Xunit;
using Microsoft.Azure.IoT.Gateway;
using System.IO;

namespace Microsoft.Azure.IoT.Gateway.Tests
{
    public class NetCoreInteropUnitTests
    {
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_001: [ If any parameter (except configuration) is null, constructor shall throw a ArgumentNullException. ] */
        [Fact]
        public void NetCoreInterop_Create_with_borker_null_arg_throw()
        {
            ///arrage

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
            ///act
            try
            {
                returnValue = NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", null);
            }
            catch (FileNotFoundException e)
            {
                ///assert
                ///
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_011: [ Receive shall get the DotNetCoreModuleInstance based on moduleID ] */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_019: [ Receive shall raise an Exception if module can not be found based on moduleID ] */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_021: [ Receive shall raise an Exception if module can't be found. ] */
        [Fact]
        public void NetCoreInterop_Receive_with_moduleid_that_not_exists_throw()
        {
            ///arrage
            ///arrage

            //Make sure we create the dictioary.
            try
            {
                NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", "AnyConfiguration");
            }
            catch (FileNotFoundException e)
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

        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_014: [ Destroy shall get the DotNetCoreModuleInstance based on moduleID ] */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_022: [ Destroy shall raise an Exception if module can't be found. ] */
        [Fact]
        public void NetCoreInterop_Destroy_with_moduleid_that_not_exists_throw()
        {
            ///arrage
            ///arrage

            //Make sure we create the dictioary.
            try
            {
                NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", "AnyConfiguration");
            }
            catch (FileNotFoundException e)
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
            var messageInstance = new Message(notFail____minimalMessage);
            uint messageSize = uint.Parse(messageInstance.Content.GetLength(0).ToString()) + 14;

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

        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_016: [ Start shall get the DotNetCoreModuleInstance based on moduleID ] */
        /* Tests_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_023: [ Start shall raise an Exception if module can't be found. ] */
        [Fact]
        public void NetCoreInterop_Start_with_moduleid_that_not_exists_throw()
        {
            ///arrage
            ///arrage

            //Make sure we create the dictioary.
            try
            {
                NetCoreInterop.Create((IntPtr)0x42, (IntPtr)0x42, "AnyAssemblyName", "AnyEntryType", "AnyConfiguration");
            }
            catch (FileNotFoundException e)
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
            var messageInstance = new Message(notFail____minimalMessage);
            uint messageSize = uint.Parse(messageInstance.Content.GetLength(0).ToString()) + 14;

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
    }
}
