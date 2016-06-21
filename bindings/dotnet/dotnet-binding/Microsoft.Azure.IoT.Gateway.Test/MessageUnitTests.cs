using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.Azure.IoT.Gateway;
using Moq;
using System.Collections;

namespace Microsoft.Azure.IoT.Gateway.Test
{

    [TestClass]
    public class MessageUnitTests
    {
        byte[] notFail____minimalMessage =
        {
            0xA1, 0x60,             /*header*/
            0x00, 0x00, 0x00, 14,   /*size of this array*/
            0x00, 0x00, 0x00, 0x00, /*zero properties*/
            0x00, 0x00, 0x00, 0x00  /*zero message content size*/
        };

        /* Tests_SRS_DOTNET_MESSAGE_04_008: [ If any parameter is null, constructor shall throw a ArgumentNullException ] */
        [TestMethod]
        public void Message_byteArrayConstructor_with_null_arg_throw()
        {
            ///arrage

            ///act
            try
            {
                var messageInstance = new Message((byte[])null);
            }            
            catch (ArgumentNullException e)
            {
                ///assert
                StringAssert.Contains(e.Message, "msgAsByteArray cannot be null");
                return;
            }
            Assert.Fail("No exception was thrown.");
            

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_008: [ If any parameter is null, constructor shall throw a ArgumentNullException ] */
        [TestMethod]
        public void Message_CopyConstructor_with_null_arg_throw()
        {
            ///arrage

            ///act
            try
            {
                var messageInstance = new Message((Message)null);
            }
            catch (ArgumentNullException e)
            {
                ///assert
                StringAssert.Contains(e.Message, "message cannot be null");
                return;
            }
            Assert.Fail("No exception was thrown.");


            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_002: [ Message class shall have a constructor that receives a byte array with it's content format as described in message_requirements.md and it's Content and Properties are extracted and saved. ] */
        [TestMethod]
        public void Message_byteArrayConstructor_minimalMessage_Succeed()
        {
            ///arrage
            byte[] notFail____minimalMessage =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 14,   /*size of this array*/
                0x00, 0x00, 0x00, 0x00, /*zero properties*/
                0x00, 0x00, 0x00, 0x00  /*zero message content size*/
            };

            ///act
            var messageInstance = new Message(notFail____minimalMessage);

            ///Assert
            Assert.AreEqual(0, messageInstance.Content.GetLength(0));
            Assert.AreEqual(0, messageInstance.Properties.Count);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_002: [ Message class shall have a constructor that receives a byte array with it's content format as described in message_requirements.md and it's Content and Properties are extracted and saved. ] */
        [TestMethod]
        public void Message_byteArrayConstructor_notFail__0Property_1bytes_Succeed()
        {
            ///arrage
            byte[] notFail__0Property_1bytes =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 15,   /*size of this array*/
                0x00, 0x00, 0x00, 0x00, /*zero properties*/
                0x00, 0x00, 0x00, 0x01,  /*1 message content size*/
                (byte)'3'
            };
            ///act
            var messageInstance = new Message(notFail__0Property_1bytes);

            ///Assert
            Assert.AreEqual(1, messageInstance.Content.GetLength(0));
            Assert.AreEqual(0, messageInstance.Properties.Count);
            Assert.AreEqual((byte)'3', messageInstance.Content[0]);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_002: [ Message class shall have a constructor that receives a byte array with it's content format as described in message_requirements.md and it's Content and Properties are extracted and saved. ] */
        [TestMethod]
        public void Message_byteArrayConstructor_notFail__0Property_2bytes_Succeed()
        {
            ///arrage
            byte[] notFail__0Property_2bytes =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 16,   /*size of this array*/
                0x00, 0x00, 0x00, 0x00, /*zero property*/
                0x00, 0x00, 0x00, 0x02,  /*2 message content size*/
                (byte)'3', (byte)'4'
            };
            ///act
            var messageInstance = new Message(notFail__0Property_2bytes);

            ///Assert
            Assert.AreEqual(2, messageInstance.Content.GetLength(0));
            Assert.AreEqual(0, messageInstance.Properties.Count);
            Assert.AreEqual((byte)'3', messageInstance.Content[0]);
            Assert.AreEqual((byte)'4', messageInstance.Content[1]);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_002: [ Message class shall have a constructor that receives a byte array with it's content format as described in message_requirements.md and it's Content and Properties are extracted and saved. ] */
        [TestMethod]
        public void Message_byteArrayConstructor_notFail__1Property_0bytes_Succeed()
        {
            ///arrage
            byte[] notFail__1Property_0bytes =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 18,   /*size of this array*/
                0x00, 0x00, 0x00, 0x01, /*one property*/
                (byte)'3', (byte)'\0', (byte)'3', (byte)'\0',
                0x00, 0x00, 0x00, 0x00  /*zero message content size*/
            };

            ///act
            var messageInstance = new Message(notFail__1Property_0bytes);

            ///Assert
            Assert.AreEqual(0, messageInstance.Content.GetLength(0));
            Assert.AreEqual(1, messageInstance.Properties.Count);
            Assert.AreEqual("3", messageInstance.Properties["3"]);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_002: [ Message class shall have a constructor that receives a byte array with it's content format as described in message_requirements.md and it's Content and Properties are extracted and saved. ] */
        [TestMethod]
        public void Message_byteArrayConstructor_notFail__1Property_1bytes_Succeed()
        {
            ///arrage
            byte[] notFail__1Property_1bytes =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 44,   /*size of this array*/
                0x00, 0x00, 0x00, 0x01, /*one property*/
                (byte)'A', (byte)'z',(byte)'u',(byte)'r',(byte)'e',(byte)' ',(byte)'I',(byte)'o',(byte)'T',(byte)' ',(byte)'G',(byte)'a',(byte)'t',(byte)'e',(byte)'w',(byte)'a',(byte)'y',(byte)' ',(byte)'i',(byte)'s',(byte)'\0',(byte)'a',(byte)'w',(byte)'e',(byte)'s',(byte)'o',(byte)'m',(byte)'e',(byte)'\0',
                0x00, 0x00, 0x00, 0x01,  /*1 message content size*/
                (byte)'3'
            };

            ///act
            var messageInstance = new Message(notFail__1Property_1bytes);

            ///Assert
            Assert.AreEqual(1, messageInstance.Content.GetLength(0));
            Assert.AreEqual(1, messageInstance.Properties.Count);
            Assert.AreEqual("awesome", messageInstance.Properties["Azure IoT Gateway is"]);
            Assert.AreEqual((byte)'3', messageInstance.Content[0]);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_002: [ Message class shall have a constructor that receives a byte array with it's content format as described in message_requirements.md and it's Content and Properties are extracted and saved. ] */
        [TestMethod]
        public void Message_byteArrayConstructor_notFail__1Property_2bytes_Succeed()
        {
            ///arrage
            byte[] notFail__1Property_2bytes =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 45,   /*size of this array*/
                0x00, 0x00, 0x00, 0x01, /*one property*/
                (byte)'A', (byte)'z',(byte)'u',(byte)'r',(byte)'e',(byte)' ',(byte)'I',(byte)'o',(byte)'T',(byte)' ',(byte)'G',(byte)'a',(byte)'t',(byte)'e',(byte)'w',(byte)'a',(byte)'y',(byte)' ',(byte)'i',(byte)'s',(byte)'\0',(byte)'a',(byte)'w',(byte)'e',(byte)'s',(byte)'o',(byte)'m',(byte)'e',(byte)'\0',
                0x00, 0x00, 0x00, 0x02,  /*2 message content size*/
                (byte)'3', (byte)'4'
            };

            ///act
            var messageInstance = new Message(notFail__1Property_2bytes);

            ///Assert
            Assert.AreEqual(2, messageInstance.Content.GetLength(0));
            Assert.AreEqual(1, messageInstance.Properties.Count);
            Assert.AreEqual("awesome", messageInstance.Properties["Azure IoT Gateway is"]);
            Assert.AreEqual((byte)'3', messageInstance.Content[0]);
            Assert.AreEqual((byte)'4', messageInstance.Content[1]);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_002: [ Message class shall have a constructor that receives a byte array with it's content format as described in message_requirements.md and it's Content and Properties are extracted and saved. ] */
        [TestMethod]
        public void Message_byteArrayConstructor_notFail__2Property_0bytes_Succeed()
        {
            ///arrage
            byte[] notFail__2Property_0bytes =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 23,   /*size of this array*/
                0x00, 0x00, 0x00, 0x02, /*two properties*/
                (byte)'3', (byte)'\0', (byte)'3', (byte)'\0',
                (byte)'a', (byte)'b', (byte)'\0', (byte)'a', (byte)'\0',
                0x00, 0x00, 0x00, 0x00  /*zero message content size*/
            };

            ///act
            var messageInstance = new Message(notFail__2Property_0bytes);

            ///Assert
            Assert.AreEqual(0, messageInstance.Content.GetLength(0));
            Assert.AreEqual(2, messageInstance.Properties.Count);
            Assert.AreEqual("3", messageInstance.Properties["3"]);
            Assert.AreEqual("a", messageInstance.Properties["ab"]);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_002: [ Message class shall have a constructor that receives a byte array with it's content format as described in message_requirements.md and it's Content and Properties are extracted and saved. ] */
        [TestMethod]
        public void Message_byteArrayConstructor_notFail__2Property_1bytes_Succeed()
        {
            ///arrage
            byte[] notFail__2Property_1bytes =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 63,   /*size of this array*/
                0x00, 0x00, 0x00, 0x02, /*two properties*/
                (byte)'A', (byte)'z',(byte)'u',(byte)'r',(byte)'e',(byte)' ',(byte)'I',(byte)'o',(byte)'T',(byte)' ',(byte)'G',(byte)'a',(byte)'t',(byte)'e',(byte)'w',(byte)'a',(byte)'y',(byte)' ',(byte)'i',(byte)'s',(byte)'\0',(byte)'a',(byte)'w',(byte)'e',(byte)'s',(byte)'o',(byte)'m',(byte)'e',(byte)'\0',
                (byte)'B',(byte)'l',(byte)'e',(byte)'e',(byte)'d',(byte)'i',(byte)'n',(byte)'g',(byte)'E',(byte)'d',(byte)'g',(byte)'e',(byte)'\0',(byte)'r',(byte)'o',(byte)'c',(byte)'k',(byte)'s',(byte)'\0',
                0x00, 0x00, 0x00, 0x01,  /*1 message content size*/
                (byte)'3'
            };

            ///act
            var messageInstance = new Message(notFail__2Property_1bytes);

            ///Assert
            Assert.AreEqual(1, messageInstance.Content.GetLength(0));
            Assert.AreEqual(2, messageInstance.Properties.Count);
            Assert.AreEqual("awesome", messageInstance.Properties["Azure IoT Gateway is"]);
            Assert.AreEqual("rocks", messageInstance.Properties["BleedingEdge"]);
            Assert.AreEqual((byte)'3', messageInstance.Content[0]);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_002: [ Message class shall have a constructor that receives a byte array with it's content format as described in message_requirements.md and it's Content and Properties are extracted and saved. ] */
        [TestMethod]
        public void Message_byteArrayConstructor_notFail__2Property_2bytes_Succeed()
        {
            ///arrage
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

            ///act
            var messageInstance = new Message(notFail__2Property_2bytes);

            ///Assert
            Assert.AreEqual(2, messageInstance.Content.GetLength(0));
            Assert.AreEqual(2, messageInstance.Properties.Count);
            Assert.AreEqual("awesome", messageInstance.Properties["Azure IoT Gateway is"]);
            Assert.AreEqual("rocks", messageInstance.Properties["BleedingEdge"]);
            Assert.AreEqual((byte)'3', messageInstance.Content[0]);
            Assert.AreEqual((byte)'4', messageInstance.Content[1]);

            ///cleanup
        }

        
        /* Tests_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [TestMethod]
        public void Message_byteArrayConstructor_when_first_byte_is_not_0xA1_throws()
        {
            ///arrage
            byte[] fail_____firstByteNot0xA1 =
            {
                0xA2, 0x60,             /*header - wrong*/
                0x00, 0x00, 0x00, 63,   /*size of this array*/
                0x00, 0x00, 0x00, 0x02, /*two properties*/
                (byte)'A', (byte)'z',(byte)'u',(byte)'r',(byte)'e',(byte)' ',(byte)'I',(byte)'o',(byte)'T',(byte)' ',(byte)'G',(byte)'a',(byte)'t',(byte)'e',(byte)'w',(byte)'a',(byte)'y',(byte)' ',(byte)'i',(byte)'s',(byte)'\0',(byte)'a',(byte)'w',(byte)'e',(byte)'s',(byte)'o',(byte)'m',(byte)'e',(byte)'\0',
                (byte)'B',(byte)'l',(byte)'e',(byte)'e',(byte)'d',(byte)'i',(byte)'n',(byte)'g',(byte)'E',(byte)'d',(byte)'g',(byte)'e',(byte)'\0',(byte)'r',(byte)'o',(byte)'c',(byte)'k',(byte)'s',(byte)'\0',
                0x00, 0x00, 0x00, 0x02,  /*1 message content size*/
                (byte)'3',(byte)'4'
            };

            ///act
            try
            {
                var messageInstance = new Message(fail_____firstByteNot0xA1);
            }
            catch (ArgumentException e)
            {
                ///assert
                StringAssert.Contains(e.Message, "Invalid Header bytes.");
                return;
            }
            Assert.Fail("No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [TestMethod]
        public void Message_byteArrayConstructor_when_second_byte_is_not_0x60_throws()
        {
            ///arrage
            byte[] fail_____secondByteNot0x60 =
            {
                0xA1, 0x64,             /*header - wrong*/
                0x00, 0x00, 0x00, 63,   /*size of this array*/
                0x00, 0x00, 0x00, 0x02, /*two properties*/
                (byte)'A', (byte)'z',(byte)'u',(byte)'r',(byte)'e',(byte)' ',(byte)'I',(byte)'o',(byte)'T',(byte)' ',(byte)'G',(byte)'a',(byte)'t',(byte)'e',(byte)'w',(byte)'a',(byte)'y',(byte)' ',(byte)'i',(byte)'s',(byte)'\0',(byte)'a',(byte)'w',(byte)'e',(byte)'s',(byte)'o',(byte)'m',(byte)'e',(byte)'\0',
                (byte)'B',(byte)'l',(byte)'e',(byte)'e',(byte)'d',(byte)'i',(byte)'n',(byte)'g',(byte)'E',(byte)'d',(byte)'g',(byte)'e',(byte)'\0',(byte)'r',(byte)'o',(byte)'c',(byte)'k',(byte)'s',(byte)'\0',
                0x00, 0x00, 0x00, 0x02,  /*1 message content size*/
                (byte)'3',(byte)'4'
            };

            ///act
            try
            {
                var messageInstance = new Message(fail_____secondByteNot0x60);
            }
            catch (ArgumentException e)
            {
                ///assert
                StringAssert.Contains(e.Message, "Invalid Header bytes.");
                return;
            }
            Assert.Fail("No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [TestMethod]
        public void Message_byteArrayConstructor_with_1_byte_of_content_size_but_2_bytes_of_content_throws()
        {
            ///arrage
            byte[] fail_whenThereIsTooMuchContent =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 24,   /*size of this array*/
                0x00, 0x00, 0x00, 0x01, /*one property*/
                (byte)'3', (byte)'3', (byte)'3', (byte)'\0',
                (byte)'3', (byte)'3', (byte)'3', (byte)'\0',
                0x00, 0x00, 0x00, 0x01,  /*1 message content size*/
                (byte)'3',(byte)'3'
            };

            ///act
            try
            {
                var messageInstance = new Message(fail_whenThereIsTooMuchContent);
            }
            catch (ArgumentException e)
            {
                ///assert
                StringAssert.Contains(e.Message, "Size of byte array doesn't match with current content.");
                return;
            }
            Assert.Fail("No exception was thrown.");

            ///cleanup
        }

        
        /* Tests_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [TestMethod]
        public void Message_byteArrayConstructor_throws_when_numberOfProperties_is_int32_max()
        {
            ///arrage
            byte[] fail_PropertyNumberIntMax =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 14,   /*size of this array*/
                0x7F, 0xFF, 0xFF, 0xFF, /*INT32_MAX properties*/
                0x00, 0x00, 0x00, 0x00  /*zero message content size*/
            };

            ///act
            try
            {
                var messageInstance = new Message(fail_PropertyNumberIntMax);
            }
            catch (ArgumentException e)
            {
                ///assert
                StringAssert.Contains(e.Message, "Number of properties can't be more than MAXINT.");
                return;
            }
            Assert.Fail("No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [TestMethod]
        public void Message_byteArrayConstructor_throws_when_numberOfProperties_is_negative()
        {
            ///arrage
            byte[] fail_PropertyNumberIntNeg =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 14,   /*size of this array*/
                0xFF, 0xFF, 0xFF, 0xFF, /*-1 properties*/
                0x00, 0x00, 0x00, 0x00  /*zero message content size*/
            };

            ///act
            try
            {
                var messageInstance = new Message(fail_PropertyNumberIntNeg);
            }
            catch (ArgumentException e)
            {
                ///assert
                StringAssert.Contains(e.Message, "Number of properties can't be negative.");
                return;
            }
            Assert.Fail("No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [TestMethod]
        public void Message_byteArrayConstructor_with_1_byte_of_content_size_but_no_content_fails_throws()
        {
            ///arrage
            byte[] fail_whenThereIsNotEnoughContent =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 22,   /*size of this array*/
                0x00, 0x00, 0x00, 0x01, /*one property*/
                (byte)'3', (byte)'3', (byte)'3', (byte)'\0',
                (byte)'3', (byte)'3', (byte)'3', (byte)'\0',
                0x00, 0x00, 0x00, 0x01  /*no further content*/
            };

            ///act
            try
            {
                var messageInstance = new Message(fail_whenThereIsNotEnoughContent);
            }
            catch (ArgumentException e)
            {
                ///assert
                StringAssert.Contains(e.Message, "Size of byte array doesn't match with current content.");
                return;
            }
            Assert.Fail("No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [TestMethod]
        public void Message_byteArrayConstructor_throws_whenThereIsOnly1ByteOfcontentSize()
        {
            ///arrage
            byte[] fail_whenThereIsOnly1ByteOfcontentSize =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 19,   /*size of this array*/
                0x00, 0x00, 0x00, 0x01, /*one property*/
                (byte)'3', (byte)'3', (byte)'3', (byte)'\0',
                (byte)'3', (byte)'3', (byte)'3', (byte)'\0',
                0x00                    /*not enough bytes for contentSize*/
            };

            ///act
            try
            {
                var messageInstance = new Message(fail_whenThereIsOnly1ByteOfcontentSize);
            }
            catch (ArgumentException e)
            {
                ///assert
                StringAssert.Contains(e.Message, "Could not read contentLength.");
                return;
            }
            Assert.Fail("No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [TestMethod]
        public void Message_byteArrayConstructor_throws_with_1_property_when_1st_property_doesnt_end_fails()
        {
            ///arrage
            byte[] fail_firstPropertyNameTooBig =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 18,   /*size of this array*/
                0x00, 0x00, 0x00, 0x01, /*one property*/
                (byte)'3', (byte)'3', (byte)'3', (byte)'3',     /*property name just keeps going...*/
                (byte)'3', (byte)'3', (byte)'3', (byte)'3'
            };

            ///act
            try
            {
                var messageInstance = new Message(fail_firstPropertyNameTooBig);
            }
            catch (ArgumentException e)
            {
                ///assert
                StringAssert.Contains(e.Message, "Could not parse Properties(key or value)");
                return;
            }
            Assert.Fail("No exception was thrown.");

            ///cleanup
        }


        /* Tests_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [TestMethod]
        public void Message_byteArrayConstructor_throws_with_1_property_when_1st_property_value_doesnt_end_fails()
        {
            ///arrage
            byte[] fail_firstPropertyValueDoesNotEnd =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 18,   /*size of this array*/
                0x00, 0x00, 0x00, 0x01, /*one property*/
                (byte)'3', (byte)'3', (byte)'3', (byte)'\0',
                (byte)'3', (byte)'3', (byte)'3', (byte)'3'     /*property value just keeps going...*/
            };

            ///act
            try
            {
                var messageInstance = new Message(fail_firstPropertyValueDoesNotEnd);
            }
            catch (ArgumentException e)
            {
                ///assert
                StringAssert.Contains(e.Message, "Could not parse Properties(key or value)");
                return;
            }
            Assert.Fail("No exception was thrown.");

            ///cleanup
        }

        
        /* Tests_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [TestMethod]
        public void Message_byteArrayConstructor_throws_with_1_property_when_1st_property_value_doesnt_start_fails()
        {
            ///arrage
            byte[] fail_firstPropertyValueDoesNotExist =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 18,   /*size of this array*/
                0x00, 0x00, 0x00, 0x01, /*one property*/
                (byte)'3', (byte)'3', (byte)'3', (byte)'3',
                (byte)'3', (byte)'3', (byte)'3', (byte)'\0'     /*property value does not have a start*/
            };

            ///act
            try
            {
                var messageInstance = new Message(fail_firstPropertyValueDoesNotExist);
            }
            catch (ArgumentException e)
            {
                ///assert
                StringAssert.Contains(e.Message, "Could not parse Properties(key or value)");
                return;
            }
            Assert.Fail("No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [TestMethod]
        public void Message_byteArrayConstructor_throws_with_13_size_fails()
        {
            ///arrage
            byte[] fail_firstPropertyValueDoesNotExist =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 13,   /*size of this array*/
                0x00, 0x00, 0x00, 0x00, /*zero properties*/
                0x00, 0x00, 0x00  /*zero message content size*/
            };

            ///act
            try
            {
                var messageInstance = new Message(fail_firstPropertyValueDoesNotExist);
            }
            catch (ArgumentException e)
            {
                ///assert
                StringAssert.Contains(e.Message, "Invalid byte array size.");
                return;
            }
            Assert.Fail("No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [TestMethod]
        public void Message_byteArrayConstructor_throws_with_2_byte_of_content_size_fails()
        {
            ///arrage
            byte[] fail_whenThereIsOnly2ByteOfcontentSize =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 20,   /*size of this array*/
                0x00, 0x00, 0x00, 0x01, /*one property*/
                (byte)'3', (byte)'3', (byte)'3', (byte)'\0',
                (byte)'3', (byte)'3', (byte)'3', (byte)'\0',
                0x00, 0x00              /*not enough bytes for contentSize*/
            };

            ///act
            try
            {
                var messageInstance = new Message(fail_whenThereIsOnly2ByteOfcontentSize);
            }
            catch (ArgumentException e)
            {
                ///assert
                StringAssert.Contains(e.Message, "Could not read contentLength.");
                return;
            }
            Assert.Fail("No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [TestMethod]
        public void Message_byteArrayConstructor_throws_with_3_byte_of_content_size_fails()
        {
            ///arrage
            byte[] fail_whenThereIsOnly3ByteOfcontentSize =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 21,   /*size of this array*/
                0x00, 0x00, 0x00, 0x01, /*one property*/
                (byte)'3', (byte)'3', (byte)'3', (byte)'\0',
                (byte)'3', (byte)'3', (byte)'3', (byte)'\0',
                0x00, 0x00, 0x00        /*not enough bytes for contentSize*/
            };

            ///act
            try
            {
                var messageInstance = new Message(fail_whenThereIsOnly3ByteOfcontentSize);
            }
            catch (ArgumentException e)
            {
                ///assert
                StringAssert.Contains(e.Message, "Could not read contentLength.");
                return;
            }
            Assert.Fail("No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_008: [ If any parameter is null, constructor shall throw a ArgumentNullException ] */
        [TestMethod]
        public void Message_StringAndPropertiesConstructor_with_null_string_throw()
        {
            ///arrage

            ///act
            try
            {
                var messageInstance = new Message((string)null, new System.Collections.Generic.Dictionary<string, string>());
            }
            catch (ArgumentNullException e)
            {
                ///assert
                StringAssert.Contains(e.Message, "content cannot be null");
                return;
            }
            Assert.Fail("No exception was thrown.");


            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_008: [ If any parameter is null, constructor shall throw a ArgumentNullException ] */
        [TestMethod]
        public void Message_StringAndPropertiesConstructor_with_null_properties_throw()
        {
            ///arrage

            ///act
            try
            {
                var messageInstance = new Message("Any String", null);
            }
            catch (ArgumentNullException e)
            {
                ///assert
                StringAssert.Contains(e.Message, "properties cannot be null");
                return;
            }
            Assert.Fail("No exception was thrown.");


            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_003: [ Message class shall have a constructor that receives a content as string and properties and store it. This string shall be converted to byte array based on System.Text.Encoding.UTF8.GetBytes(). ] */
        [TestMethod]
        public void Message_StringAndPropertiesConstructor_ValidContentString_0Properties_Succeed()
        {
            ///arrage

            ///act
            var messageInstance = new Message("Any Message Content as String", new System.Collections.Generic.Dictionary<string, string>());

            Assert.AreEqual("Any Message Content as String", System.Text.Encoding.UTF8.GetString(messageInstance.Content));
            Assert.AreEqual(0, messageInstance.Properties.Count);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_003: [ Message class shall have a constructor that receives a content as string and properties and store it. This string shall be converted to byte array based on System.Text.Encoding.UTF8.GetBytes(). ] */
        [TestMethod]
        public void Message_StringAndPropertiesConstructor_EmptyContentString_0Properties_Succeed()
        {
            ///arrage

            ///act
            var messageInstance = new Message("", new System.Collections.Generic.Dictionary<string, string>());

            Assert.AreEqual("", System.Text.Encoding.UTF8.GetString(messageInstance.Content));
            Assert.AreEqual(0, messageInstance.Properties.Count);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_003: [ Message class shall have a constructor that receives a content as string and properties and store it. This string shall be converted to byte array based on System.Text.Encoding.UTF8.GetBytes(). ] */
        [TestMethod]
        public void Message_StringAndPropertiesConstructor_ValidString_1Properties_Succeed()
        {
            ///arrage
            var inputProperties = new System.Collections.Generic.Dictionary<string, string>();

            inputProperties.Add("Prop1", "Value1");

            ///act
            var messageInstance = new Message("Any Content String", inputProperties);

            Assert.AreEqual("Any Content String", System.Text.Encoding.UTF8.GetString(messageInstance.Content));
            Assert.AreEqual(1, messageInstance.Properties.Count);
            Assert.AreEqual("Value1", messageInstance.Properties["Prop1"]);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_003: [ Message class shall have a constructor that receives a content as string and properties and store it. This string shall be converted to byte array based on System.Text.Encoding.UTF8.GetBytes(). ] */
        [TestMethod]
        public void Message_StringAndPropertiesConstructor_ValidString_2Properties_Succeed()
        {
            ///arrage
            var inputProperties = new System.Collections.Generic.Dictionary<string, string>();

            inputProperties.Add("Prop1", "Value1");
            inputProperties.Add("Prop2", "Value2");

            ///act
            var messageInstance = new Message("Any Content String", inputProperties);

            Assert.AreEqual("Any Content String", System.Text.Encoding.UTF8.GetString(messageInstance.Content));
            Assert.AreEqual(2, messageInstance.Properties.Count);
            Assert.AreEqual("Value1", messageInstance.Properties["Prop1"]);
            Assert.AreEqual("Value2", messageInstance.Properties["Prop2"]);

            ///cleanup
        }
        /* Tests_SRS_DOTNET_MESSAGE_04_008: [ If any parameter is null, constructor shall throw a ArgumentNullException ] */
        [TestMethod]
        public void Message_bytearrayAndPropertiesContructor_with_null_content_throw()
        {
            ///arrage

            ///act
            try
            {
                var messageInstance = new Message((byte[])null, new System.Collections.Generic.Dictionary<string, string>());
            }
            catch (ArgumentNullException e)
            {
                ///assert
                StringAssert.Contains(e.Message, "contentAsByteArray cannot be null");
                return;
            }
            Assert.Fail("No exception was thrown.");


            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_008: [ If any parameter is null, constructor shall throw a ArgumentNullException ] */
        [TestMethod]
        public void Message_bytearrayAndPropertiesContructor_with_null_Properties_throw()
        {
            ///arrage

            ///act
            try
            {
                var messageInstance = new Message(new byte[0], null);
            }
            catch (ArgumentNullException e)
            {
                ///assert
                StringAssert.Contains(e.Message, "properties cannot be null");
                return;
            }
            Assert.Fail("No exception was thrown.");


            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_004: [ Message class shall have a constructor that receives a content as byte[] and properties, storing them. ] */
        [TestMethod]
        public void Message_bytearrayAndPropertiesContructor_0bytes_0Properties_succeed()
        {
            ///arrage
            var inputByteArray = new byte[0];
            ///act
            var messageInstance = new Message(inputByteArray, new System.Collections.Generic.Dictionary<string, string>());

            Assert.AreEqual(inputByteArray, messageInstance.Content);
            Assert.AreEqual(0, messageInstance.Properties.Count);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_004: [ Message class shall have a constructor that receives a content as byte[] and properties, storing them. ] */
        [TestMethod]
        public void Message_bytearrayAndPropertiesContructor_1bytes_0Properties_succeed()
        {
            ///arrage
            var inputByteArray = new byte[1];
            inputByteArray[0] = (byte)'A';
            ///act
            var messageInstance = new Message(inputByteArray, new System.Collections.Generic.Dictionary<string, string>());

            Assert.AreEqual(inputByteArray, messageInstance.Content);
            Assert.AreEqual(inputByteArray[0], messageInstance.Content[0]);
            Assert.AreEqual(0, messageInstance.Properties.Count);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_004: [ Message class shall have a constructor that receives a content as byte[] and properties, storing them. ] */
        [TestMethod]
        public void Message_bytearrayAndPropertiesContructor_1bytes_1Properties_succeed()
        {
            ///arrage
            var inputByteArray = new byte[1];
            inputByteArray[0] = (byte)'A';
            var inputProperties = new System.Collections.Generic.Dictionary<string, string>();

            inputProperties.Add("Prop1", "Value1");

            ///act
            var messageInstance = new Message(inputByteArray, inputProperties);

            Assert.AreEqual(inputByteArray, messageInstance.Content);
            Assert.AreEqual(inputByteArray[0], messageInstance.Content[0]);
            Assert.AreEqual(1, messageInstance.Properties.Count);
            Assert.AreEqual("Value1", messageInstance.Properties["Prop1"]);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_004: [ Message class shall have a constructor that receives a content as byte[] and properties, storing them. ] */
        [TestMethod]
        public void Message_bytearrayAndPropertiesContructor_1bytes_2Properties_succeed()
        {
            ///arrage
            var inputByteArray = new byte[1];
            inputByteArray[0] = (byte)'A';
            var inputProperties = new System.Collections.Generic.Dictionary<string, string>();

            inputProperties.Add("Prop1", "Value1");
            inputProperties.Add("Prop2", "Value2");

            ///act
            var messageInstance = new Message(inputByteArray, inputProperties);

            Assert.AreEqual(inputByteArray, messageInstance.Content);
            Assert.AreEqual(inputByteArray[0], messageInstance.Content[0]);
            Assert.AreEqual(2, messageInstance.Properties.Count);
            Assert.AreEqual("Value1", messageInstance.Properties["Prop1"]);
            Assert.AreEqual("Value2", messageInstance.Properties["Prop2"]);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_005: [ Message Class shall have a ToByteArray method which will convert it's byte array Content and it's Properties to a byte[] which format is described at message_requirements.md ] */
        [TestMethod]
        public void Message_ToByteArray_noProperties_no_content_happy_path()
        {
            ///arrage
            var inputByteArray = new byte[0];
            var inputProperties = new System.Collections.Generic.Dictionary<string, string>();
            var messageInstance = new Message(inputByteArray, inputProperties);

            byte[] notFail____minimalMessage =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 14,   /*size of this array*/
                0x00, 0x00, 0x00, 0x00, /*zero properties*/
                0x00, 0x00, 0x00, 0x00  /*zero message content size*/
            };

            ///act

            var byteArrayInstance = messageInstance.ToByteArray();

            Assert.AreEqual(14, byteArrayInstance.Length);
            Assert.IsTrue(StructuralComparisons.StructuralEqualityComparer.Equals(notFail____minimalMessage,byteArrayInstance));


            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_005: [ Message Class shall have a ToByteArray method which will convert it's byte array Content and it's Properties to a byte[] which format is described at message_requirements.md ] */
        [TestMethod]
        public void Message_ToByteArray_with_0Property_1bytes_content_happy_path()
        {
            ///arrage
            byte[] notFail__0Property_1bytes =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 15,   /*size of this array*/
                0x00, 0x00, 0x00, 0x00, /*zero properties*/
                0x00, 0x00, 0x00, 0x01,  /*1 message content size*/
                (byte)'3'
            };
            var messageInstance = new Message(notFail__0Property_1bytes);

            ///act

            var byteArrayInstance = messageInstance.ToByteArray();

            Assert.AreEqual(15, byteArrayInstance.Length);
            Assert.IsTrue(StructuralComparisons.StructuralEqualityComparer.Equals(notFail__0Property_1bytes, byteArrayInstance));


            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_005: [ Message Class shall have a ToByteArray method which will convert it's byte array Content and it's Properties to a byte[] which format is described at message_requirements.md ] */
        [TestMethod]
        public void Message_ToByteArray_with_0Property_2bytes_happy_path()
        {
            ///arrage
            byte[] notFail__0Property_2bytes =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 16,   /*size of this array*/
                0x00, 0x00, 0x00, 0x00, /*zero property*/
                0x00, 0x00, 0x00, 0x02,  /*2 message content size*/
                (byte)'3', (byte)'4'
            };
            var messageInstance = new Message(notFail__0Property_2bytes);

            ///act

            var byteArrayInstance = messageInstance.ToByteArray();

            Assert.AreEqual(16, byteArrayInstance.Length);
            Assert.IsTrue(StructuralComparisons.StructuralEqualityComparer.Equals(notFail__0Property_2bytes, byteArrayInstance));


            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_005: [ Message Class shall have a ToByteArray method which will convert it's byte array Content and it's Properties to a byte[] which format is described at message_requirements.md ] */
        [TestMethod]
        public void Message_ToByteArray_with_1Property_0bytes_happy_path()
        {
            ///arrage
            byte[] notFail__1Property_0bytes =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 18,   /*size of this array*/
                0x00, 0x00, 0x00, 0x01, /*one property*/
                (byte)'3', (byte)'\0', (byte)'3', (byte)'\0',
                0x00, 0x00, 0x00, 0x00  /*zero message content size*/
            };
            var messageInstance = new Message(notFail__1Property_0bytes);

            ///act

            var byteArrayInstance = messageInstance.ToByteArray();

            Assert.AreEqual(18, byteArrayInstance.Length);
            Assert.IsTrue(StructuralComparisons.StructuralEqualityComparer.Equals(notFail__1Property_0bytes, byteArrayInstance));


            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_005: [ Message Class shall have a ToByteArray method which will convert it's byte array Content and it's Properties to a byte[] which format is described at message_requirements.md ] */
        [TestMethod]
        public void Message_ToByteArray_with_1Property_1bytes_happy_path()
        {
            ///arrage
            byte[] notFail__1Property_1bytes =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 44,   /*size of this array*/
                0x00, 0x00, 0x00, 0x01, /*one property*/
                (byte)'A', (byte)'z',(byte)'u',(byte)'r',(byte)'e',(byte)' ',(byte)'I',(byte)'o',(byte)'T',(byte)' ',(byte)'G',(byte)'a',(byte)'t',(byte)'e',(byte)'w',(byte)'a',(byte)'y',(byte)' ',(byte)'i',(byte)'s',(byte)'\0',(byte)'a',(byte)'w',(byte)'e',(byte)'s',(byte)'o',(byte)'m',(byte)'e',(byte)'\0',
                0x00, 0x00, 0x00, 0x01,  /*1 message content size*/
                (byte)'3'
            };
            var messageInstance = new Message(notFail__1Property_1bytes);

            ///act

            var byteArrayInstance = messageInstance.ToByteArray();

            Assert.AreEqual(44, byteArrayInstance.Length);
            Assert.IsTrue(StructuralComparisons.StructuralEqualityComparer.Equals(notFail__1Property_1bytes, byteArrayInstance));


            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_005: [ Message Class shall have a ToByteArray method which will convert it's byte array Content and it's Properties to a byte[] which format is described at message_requirements.md ] */
        [TestMethod]
        public void Message_ToByteArray_with_1Property_2bytes_happy_path()
        {
            ///arrage
            byte[] notFail__1Property_2bytes =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 45,   /*size of this array*/
                0x00, 0x00, 0x00, 0x01, /*one property*/
                (byte)'A', (byte)'z',(byte)'u',(byte)'r',(byte)'e',(byte)' ',(byte)'I',(byte)'o',(byte)'T',(byte)' ',(byte)'G',(byte)'a',(byte)'t',(byte)'e',(byte)'w',(byte)'a',(byte)'y',(byte)' ',(byte)'i',(byte)'s',(byte)'\0',(byte)'a',(byte)'w',(byte)'e',(byte)'s',(byte)'o',(byte)'m',(byte)'e',(byte)'\0',
                0x00, 0x00, 0x00, 0x02,  /*2 message content size*/
                (byte)'3', (byte)'4'
            };
            var messageInstance = new Message(notFail__1Property_2bytes);

            ///act

            var byteArrayInstance = messageInstance.ToByteArray();

            Assert.AreEqual(45, byteArrayInstance.Length);
            Assert.IsTrue(StructuralComparisons.StructuralEqualityComparer.Equals(notFail__1Property_2bytes, byteArrayInstance));


            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_005: [ Message Class shall have a ToByteArray method which will convert it's byte array Content and it's Properties to a byte[] which format is described at message_requirements.md ] */
        [TestMethod]
        public void Message_ToByteArray_with_2Property_0bytes_happy_path()
        {
            ///arrage
            byte[] notFail__2Property_0bytes =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 23,   /*size of this array*/
                0x00, 0x00, 0x00, 0x02, /*two properties*/
                (byte)'3', (byte)'\0', (byte)'3', (byte)'\0',
                (byte)'a', (byte)'b', (byte)'\0', (byte)'a', (byte)'\0',
                0x00, 0x00, 0x00, 0x00  /*zero message content size*/
            };
            var messageInstance = new Message(notFail__2Property_0bytes);

            ///act

            var byteArrayInstance = messageInstance.ToByteArray();

            Assert.AreEqual(23, byteArrayInstance.Length);
            Assert.IsTrue(StructuralComparisons.StructuralEqualityComparer.Equals(notFail__2Property_0bytes, byteArrayInstance));


            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_005: [ Message Class shall have a ToByteArray method which will convert it's byte array Content and it's Properties to a byte[] which format is described at message_requirements.md ] */
        [TestMethod]
        public void Message_ToByteArray_with_2Property_1bytes_happy_path()
        {
            ///arrage
            byte[] notFail__2Property_1bytes =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 63,   /*size of this array*/
                0x00, 0x00, 0x00, 0x02, /*two properties*/
                (byte)'A', (byte)'z',(byte)'u',(byte)'r',(byte)'e',(byte)' ',(byte)'I',(byte)'o',(byte)'T',(byte)' ',(byte)'G',(byte)'a',(byte)'t',(byte)'e',(byte)'w',(byte)'a',(byte)'y',(byte)' ',(byte)'i',(byte)'s',(byte)'\0',(byte)'a',(byte)'w',(byte)'e',(byte)'s',(byte)'o',(byte)'m',(byte)'e',(byte)'\0',
                (byte)'B',(byte)'l',(byte)'e',(byte)'e',(byte)'d',(byte)'i',(byte)'n',(byte)'g',(byte)'E',(byte)'d',(byte)'g',(byte)'e',(byte)'\0',(byte)'r',(byte)'o',(byte)'c',(byte)'k',(byte)'s',(byte)'\0',
                0x00, 0x00, 0x00, 0x01,  /*1 message content size*/
                (byte)'3'
            };
            var messageInstance = new Message(notFail__2Property_1bytes);

            ///act

            var byteArrayInstance = messageInstance.ToByteArray();

            Assert.AreEqual(63, byteArrayInstance.Length);
            Assert.IsTrue(StructuralComparisons.StructuralEqualityComparer.Equals(notFail__2Property_1bytes, byteArrayInstance));


            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_005: [ Message Class shall have a ToByteArray method which will convert it's byte array Content and it's Properties to a byte[] which format is described at message_requirements.md ] */
        [TestMethod]
        public void Message_ToByteArray_with_2Property_2bytes_happy_path()
        {
            ///arrage
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
            var messageInstance = new Message(notFail__2Property_2bytes);

            ///act

            var byteArrayInstance = messageInstance.ToByteArray();

            Assert.AreEqual(64, byteArrayInstance.Length);
            Assert.IsTrue(StructuralComparisons.StructuralEqualityComparer.Equals(notFail__2Property_2bytes, byteArrayInstance));


            ///cleanup
        }

        /* Tests_SRS_DOTNET_MESSAGE_04_005: [ Message Class shall have a ToByteArray method which will convert it's byte array Content and it's Properties to a byte[] which format is described at message_requirements.md ] */
        [TestMethod]
        public void Message_ToByteArray_with_2properties_and_2bytes_content_happy_path()
        {
            ///arrage
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
            var messageInstance = new Message(notFail__2Property_2bytes);

            ///act

            var byteArrayInstance = messageInstance.ToByteArray();

            Assert.AreEqual(64, byteArrayInstance.Length);
            Assert.IsTrue(StructuralComparisons.StructuralEqualityComparer.Equals(notFail__2Property_2bytes, byteArrayInstance));


            ///cleanup
        }

    }
}




