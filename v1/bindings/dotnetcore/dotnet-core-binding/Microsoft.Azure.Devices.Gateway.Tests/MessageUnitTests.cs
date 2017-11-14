using System;
using Xunit;
using Microsoft.Azure.Devices.Gateway;
using System.Collections;

namespace Microsoft.Azure.Devices.Gateway.Test
{
    public class MessageUnitTests
    {
        //Utility implementation
        private byte[] buildMsgAsByteArray(int desiredNumberOfProperties, int desiredContentSize)
        {
            Random randomize = new Random();

            //1-Calculate the size of the array;
            int sizeOfArray = 2 + 4 + 4 + (13 * desiredNumberOfProperties) + 4 + desiredContentSize;

            //2-Create the byte array;
            byte[] returnByteArray = new Byte[sizeOfArray];
            int currentIndex = 0;

            //3-Fill the first 2 bytes with 0xA1 and 0x60
            returnByteArray[currentIndex++] = 0xA1;
            returnByteArray[currentIndex++] = 0x60;

            //4-Fill the 4 bytes with the array size;
            byte[] sizeOfArrayByteArray = BitConverter.GetBytes(sizeOfArray);
            Array.Reverse(sizeOfArrayByteArray); //Have to reverse because this is not MSB and needs to be.
            returnByteArray[currentIndex++] = sizeOfArrayByteArray[0];
            returnByteArray[currentIndex++] = sizeOfArrayByteArray[1];
            returnByteArray[currentIndex++] = sizeOfArrayByteArray[2];
            returnByteArray[currentIndex++] = sizeOfArrayByteArray[3];

            //5-Fill the 4 bytes with the amount of properties;
            byte[] numberOfPropertiesInByteArray = BitConverter.GetBytes(desiredNumberOfProperties);
            Array.Reverse(numberOfPropertiesInByteArray); //Have to reverse because this is not MSB and needs to be. 
            returnByteArray[currentIndex++] = numberOfPropertiesInByteArray[0];
            returnByteArray[currentIndex++] = numberOfPropertiesInByteArray[1];
            returnByteArray[currentIndex++] = numberOfPropertiesInByteArray[2];
            returnByteArray[currentIndex++] = numberOfPropertiesInByteArray[3];


            //6-Fill the bytes with content from key/value of properties (null terminated string separated);

            //Fill properties with random content.
            for (int i = 0; i < desiredNumberOfProperties; i++)
            {
                //1st translate the number into string. 
                string iString = (i + 1).ToString();
                char[] key = iString.ToCharArray();

                switch (key.Length)
                {
                    case 1:
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)key[0];
                        break;
                    case 2:
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)key[0];
                        returnByteArray[currentIndex++] = (byte)key[1];
                        break;
                    case 3:
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)key[0];
                        returnByteArray[currentIndex++] = (byte)key[1];
                        returnByteArray[currentIndex++] = (byte)key[2];
                        break;
                    case 4:
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)key[0];
                        returnByteArray[currentIndex++] = (byte)key[1];
                        returnByteArray[currentIndex++] = (byte)key[2];
                        returnByteArray[currentIndex++] = (byte)key[3];
                        break;
                    case 5:
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)key[0];
                        returnByteArray[currentIndex++] = (byte)key[1];
                        returnByteArray[currentIndex++] = (byte)key[2];
                        returnByteArray[currentIndex++] = (byte)key[3];
                        returnByteArray[currentIndex++] = (byte)key[4];
                        break;
                    case 6:
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)key[0];
                        returnByteArray[currentIndex++] = (byte)key[1];
                        returnByteArray[currentIndex++] = (byte)key[2];
                        returnByteArray[currentIndex++] = (byte)key[3];
                        returnByteArray[currentIndex++] = (byte)key[4];
                        returnByteArray[currentIndex++] = (byte)key[5];
                        break;
                    case 7:
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)key[0];
                        returnByteArray[currentIndex++] = (byte)key[1];
                        returnByteArray[currentIndex++] = (byte)key[2];
                        returnByteArray[currentIndex++] = (byte)key[3];
                        returnByteArray[currentIndex++] = (byte)key[4];
                        returnByteArray[currentIndex++] = (byte)key[5];
                        returnByteArray[currentIndex++] = (byte)key[6];
                        break;
                    case 8:
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)key[0];
                        returnByteArray[currentIndex++] = (byte)key[1];
                        returnByteArray[currentIndex++] = (byte)key[2];
                        returnByteArray[currentIndex++] = (byte)key[3];
                        returnByteArray[currentIndex++] = (byte)key[4];
                        returnByteArray[currentIndex++] = (byte)key[5];
                        returnByteArray[currentIndex++] = (byte)key[6];
                        returnByteArray[currentIndex++] = (byte)key[7];
                        break;
                    case 9:
                        returnByteArray[currentIndex++] = (byte)'0';
                        returnByteArray[currentIndex++] = (byte)key[0];
                        returnByteArray[currentIndex++] = (byte)key[1];
                        returnByteArray[currentIndex++] = (byte)key[2];
                        returnByteArray[currentIndex++] = (byte)key[3];
                        returnByteArray[currentIndex++] = (byte)key[4];
                        returnByteArray[currentIndex++] = (byte)key[5];
                        returnByteArray[currentIndex++] = (byte)key[6];
                        returnByteArray[currentIndex++] = (byte)key[7];
                        returnByteArray[currentIndex++] = (byte)key[8];
                        break;
                    case 10: //This is for 2G size of message with parameter only
                        returnByteArray[currentIndex++] = (byte)key[0];
                        returnByteArray[currentIndex++] = (byte)key[1];
                        returnByteArray[currentIndex++] = (byte)key[2];
                        returnByteArray[currentIndex++] = (byte)key[3];
                        returnByteArray[currentIndex++] = (byte)key[4];
                        returnByteArray[currentIndex++] = (byte)key[5];
                        returnByteArray[currentIndex++] = (byte)key[6];
                        returnByteArray[currentIndex++] = (byte)key[7];
                        returnByteArray[currentIndex++] = (byte)key[8];
                        returnByteArray[currentIndex++] = (byte)key[9];
                        break;
                    default:
                        throw new Exception("Maximum number of property reached");
                }

                returnByteArray[currentIndex++] = 0;
                returnByteArray[currentIndex++] = (byte)randomize.Next(1, 254);
                returnByteArray[currentIndex++] = 0;
            }
            //7-Fill the amount of bytes on the content in 4 bytes after the properties; 
            byte[] contentSizeInByteArray = BitConverter.GetBytes(desiredContentSize);
            Array.Reverse(contentSizeInByteArray); //Have to reverse because this is not MSB and needs to be. 
            returnByteArray[currentIndex++] = contentSizeInByteArray[0];
            returnByteArray[currentIndex++] = contentSizeInByteArray[1];
            returnByteArray[currentIndex++] = contentSizeInByteArray[2];
            returnByteArray[currentIndex++] = contentSizeInByteArray[3];

            for (int i = 0; i < desiredContentSize; i++)
            {
                returnByteArray[currentIndex++] = (byte)randomize.Next(1, 254);
            }

            return returnByteArray;
        }


        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_008: [ If any parameter is null, constructor shall throw a ArgumentNullException ] */
        [Fact]
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
                ///
                Assert.Contains("msgAsByteArray cannot be null", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown."); 
            
            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_008: [ If any parameter is null, constructor shall throw a ArgumentNullException ] */
        [Fact]
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
                Assert.Contains("message cannot be null",e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");


            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_002: [ Message class shall have a constructor that receives a byte array with it's content format as described in message_requirements.md and it's Content and Properties are extracted and saved. ] */
        [Fact]
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
            Assert.Equal(0, messageInstance.Content.GetLength(0));
            Assert.Equal(0, messageInstance.Properties.Count);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_002: [ Message class shall have a constructor that receives a byte array with it's content format as described in message_requirements.md and it's Content and Properties are extracted and saved. ] */
        [Fact]
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
            Assert.Equal(1, messageInstance.Content.GetLength(0));
            Assert.Equal(0, messageInstance.Properties.Count);
            Assert.Equal((byte)'3', messageInstance.Content[0]);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_002: [ Message class shall have a constructor that receives a byte array with it's content format as described in message_requirements.md and it's Content and Properties are extracted and saved. ] */
        [Fact]
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
            Assert.Equal(2, messageInstance.Content.GetLength(0));
            Assert.Equal(0, messageInstance.Properties.Count);
            Assert.Equal((byte)'3', messageInstance.Content[0]);
            Assert.Equal((byte)'4', messageInstance.Content[1]);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_002: [ Message class shall have a constructor that receives a byte array with it's content format as described in message_requirements.md and it's Content and Properties are extracted and saved. ] */
        [Fact]
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
            Assert.Equal(0, messageInstance.Content.GetLength(0));
            Assert.Equal(1, messageInstance.Properties.Count);
            Assert.Equal("3", messageInstance.Properties["3"]);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_002: [ Message class shall have a constructor that receives a byte array with it's content format as described in message_requirements.md and it's Content and Properties are extracted and saved. ] */
        [Fact]
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
            Assert.Equal(1, messageInstance.Content.GetLength(0));
            Assert.Equal(1, messageInstance.Properties.Count);
            Assert.Equal("awesome", messageInstance.Properties["Azure IoT Gateway is"]);
            Assert.Equal((byte)'3', messageInstance.Content[0]);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_002: [ Message class shall have a constructor that receives a byte array with it's content format as described in message_requirements.md and it's Content and Properties are extracted and saved. ] */
        [Fact]
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
            Assert.Equal(2, messageInstance.Content.GetLength(0));
            Assert.Equal(1, messageInstance.Properties.Count);
            Assert.Equal("awesome", messageInstance.Properties["Azure IoT Gateway is"]);
            Assert.Equal((byte)'3', messageInstance.Content[0]);
            Assert.Equal((byte)'4', messageInstance.Content[1]);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_002: [ Message class shall have a constructor that receives a byte array with it's content format as described in message_requirements.md and it's Content and Properties are extracted and saved. ] */
        [Fact]
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
            Assert.Equal(0, messageInstance.Content.GetLength(0));
            Assert.Equal(2, messageInstance.Properties.Count);
            Assert.Equal("3", messageInstance.Properties["3"]);
            Assert.Equal("a", messageInstance.Properties["ab"]);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_002: [ Message class shall have a constructor that receives a byte array with it's content format as described in message_requirements.md and it's Content and Properties are extracted and saved. ] */
        [Fact]
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
            Assert.Equal(1, messageInstance.Content.GetLength(0));
            Assert.Equal(2, messageInstance.Properties.Count);
            Assert.Equal("awesome", messageInstance.Properties["Azure IoT Gateway is"]);
            Assert.Equal("rocks", messageInstance.Properties["BleedingEdge"]);
            Assert.Equal((byte)'3', messageInstance.Content[0]);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_002: [ Message class shall have a constructor that receives a byte array with it's content format as described in message_requirements.md and it's Content and Properties are extracted and saved. ] */
        [Fact]
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
            Assert.Equal(2, messageInstance.Content.GetLength(0));
            Assert.Equal(2, messageInstance.Properties.Count);
            Assert.Equal("awesome", messageInstance.Properties["Azure IoT Gateway is"]);
            Assert.Equal("rocks", messageInstance.Properties["BleedingEdge"]);
            Assert.Equal((byte)'3', messageInstance.Content[0]);
            Assert.Equal((byte)'4', messageInstance.Content[1]);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_002: [ Message class shall have a constructor that receives a byte array with it's content format as described in message_requirements.md and it's Content and Properties are extracted and saved. ] */
        [Fact]
        public void Message_byteArrayConstructor_notFail__1000Properties_count_2bytes_Succeed()
        {
            ///arrage
            byte[] msgToTest = buildMsgAsByteArray(1000, 2);

            ///act
            var messageInstance = new Message(msgToTest);

            ///Assert
            Assert.Equal(2, messageInstance.Content.GetLength(0));
            Assert.Equal(1000, messageInstance.Properties.Count);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_002: [ Message class shall have a constructor that receives a byte array with it's content format as described in message_requirements.md and it's Content and Properties are extracted and saved. ] */
        [Fact]
        public void Message_byteArrayConstructor_notFail__UnicodeKeyAndValueProperty_2bytes_Succeed()
        {
            ///arrage
            byte[] notFail__UnicodeValueAndKeyProperty_2bytes =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 36,   /*size of this array*/
                0x00, 0x00, 0x00, 0x01, /*two properties*/
                0xED, 0x95, 0x9C, 0xEA, 0xB5, 0xAD, 0xEC, 0x96, 0xB4, 0x00,
                0xE6, 0x97, 0xA5, 0xE6, 0x9C, 0xAC, 0xE8, 0xAA, 0x9E, 0x00,
                0x00, 0x00, 0x00, 0x02,  /*1 message content size*/
                (byte)'3',(byte)'4'
            };

            ///act
            var messageInstance = new Message(notFail__UnicodeValueAndKeyProperty_2bytes);

            ///Assert
            Assert.Equal(2, messageInstance.Content.GetLength(0));
            Assert.Equal(1, messageInstance.Properties.Count);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_002: [ Message class shall have a constructor that receives a byte array with it's content format as described in message_requirements.md and it's Content and Properties are extracted and saved. ] */
        [Fact]
        public void Message_byteArrayConstructor_notFail__1Properties_count_2MBbytes_Succeed()
        {
            ///arrage
            byte[] msgToTest = buildMsgAsByteArray(1, 2000000);

            ///act
            var messageInstance = new Message(msgToTest);

            ///Assert
            Assert.Equal(2000000, messageInstance.Content.GetLength(0));
            Assert.Equal(1, messageInstance.Properties.Count);

            ///cleanup
        }



        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [Fact]
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
                Assert.Contains("Invalid Header bytes.", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [Fact]
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
                Assert.Contains("Invalid Header bytes.", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [Fact]
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
                Assert.Contains("Size of byte array doesn't match with current content.", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }


        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [Fact]
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
                Assert.Contains("Number of properties can't be negative.", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [Fact]
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
                Assert.Contains("Size of byte array doesn't match with current content.", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [Fact]
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
                Assert.Contains("Could not read contentLength.", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [Fact]
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
                Assert.Contains("Could not parse Properties(key)", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }


        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [Fact]
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
                Assert.Contains("Could not parse Properties(value)", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }


        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [Fact]
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
                Assert.Contains("Could not parse Properties(value)", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [Fact]
        public void Message_byteArrayConstructor_throws_with_1_propertycount_but_no_property_present()
        {
            ///arrage
            byte[] fail_propertyCount1ButNoProperty =
            {
                0xA1, 0x60,             /*header*/
                0x00, 0x00, 0x00, 10,   /*size of this array*/
                0x00, 0x00, 0x00, 0x01 /*one property*/
            };

            ///act
            try
            {
                var messageInstance = new Message(fail_propertyCount1ButNoProperty);
            }
            catch (ArgumentException e)
            {
                ///assert
                Assert.Contains("Invalid byte array size.", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [Fact]
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
                Assert.Contains("Invalid byte array size.", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [Fact]
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
                Assert.Contains("Could not read contentLength.", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [Fact]
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
                Assert.Contains("Could not read contentLength.", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [Fact]
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
                Assert.Contains("Number of properties can't be more than MAXINT.", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
        [Fact]
        public void Message_byteArrayConstructor_throws_when_msgSizeif_int32_max()
        {
            ///arrage
            byte[] fail_PropertyNumberIntMax =
            {
                0xA1, 0x60,             /*header*/
                0x7F, 0xFF, 0xFF, 0xFF,   /*size of this array is INT32_MAX*/
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
                Assert.Contains("Size of MsgArray can't be more than MAXINT.", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");

            ///cleanup
        }


        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_008: [ If any parameter is null, constructor shall throw a ArgumentNullException ] */
        [Fact]
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
                Assert.Contains("content cannot be null", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");


            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_008: [ If any parameter is null, constructor shall throw a ArgumentNullException ] */
        [Fact]
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
                Assert.Contains("properties cannot be null", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown.");


            ///cleanup
        }


        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_003: [ Message class shall have a constructor that receives a content as string and properties and store it. This string shall be converted to byte array based on System.Text.Encoding.UTF8.GetBytes(). ] */
        [Fact]
        public void Message_StringAndPropertiesConstructor_ValidContentString_0Properties_Succeed()
        {
            ///arrage

            ///act
            var messageInstance = new Message("Any Message Content as String", new System.Collections.Generic.Dictionary<string, string>());

            Assert.Equal("Any Message Content as String", System.Text.Encoding.UTF8.GetString(messageInstance.Content));
            Assert.Equal(0, messageInstance.Properties.Count);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_003: [ Message class shall have a constructor that receives a content as string and properties and store it. This string shall be converted to byte array based on System.Text.Encoding.UTF8.GetBytes(). ] */
        [Fact]
        public void Message_StringAndPropertiesConstructor_EmptyContentString_0Properties_Succeed()
        {
            ///arrage

            ///act
            var messageInstance = new Message("", new System.Collections.Generic.Dictionary<string, string>());

            Assert.Equal("", System.Text.Encoding.UTF8.GetString(messageInstance.Content));
            Assert.Equal(0, messageInstance.Properties.Count);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_003: [ Message class shall have a constructor that receives a content as string and properties and store it. This string shall be converted to byte array based on System.Text.Encoding.UTF8.GetBytes(). ] */
        [Fact]
        public void Message_StringAndPropertiesConstructor_ValidString_1Properties_Succeed()
        {
            ///arrage
            var inputProperties = new System.Collections.Generic.Dictionary<string, string>();

            inputProperties.Add("Prop1", "Value1");

            ///act
            var messageInstance = new Message("Any Content String", inputProperties);

            Assert.Equal("Any Content String", System.Text.Encoding.UTF8.GetString(messageInstance.Content));
            Assert.Equal(1, messageInstance.Properties.Count);
            Assert.Equal("Value1", messageInstance.Properties["Prop1"]);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_003: [ Message class shall have a constructor that receives a content as string and properties and store it. This string shall be converted to byte array based on System.Text.Encoding.UTF8.GetBytes(). ] */
        [Fact]
        public void Message_StringAndPropertiesConstructor_ValidString_2Properties_Succeed()
        {
            ///arrage
            var inputProperties = new System.Collections.Generic.Dictionary<string, string>();

            inputProperties.Add("Prop1", "Value1");
            inputProperties.Add("Prop2", "Value2");

            ///act
            var messageInstance = new Message("Any Content String", inputProperties);

            Assert.Equal("Any Content String", System.Text.Encoding.UTF8.GetString(messageInstance.Content));
            Assert.Equal(2, messageInstance.Properties.Count);
            Assert.Equal("Value1", messageInstance.Properties["Prop1"]);
            Assert.Equal("Value2", messageInstance.Properties["Prop2"]);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_003: [ Message class shall have a constructor that receives a content as string and properties and store it. This string shall be converted to byte array based on System.Text.Encoding.UTF8.GetBytes(). ] */
        [Fact]
        public void Message_StringAndPropertiesConstructor_UniCode_Properties_Succeed()
        {
            ///arrage
            var inputProperties = new System.Collections.Generic.Dictionary<string, string>();

            inputProperties.Add("辉煌的混蛋", "辉煌的混蛋");
            inputProperties.Add("辉煌的混", "辉煌的混");

            ///act
            var messageInstance = new Message("辉煌的混蛋", inputProperties);

            Assert.Equal("辉煌的混蛋", System.Text.Encoding.UTF8.GetString(messageInstance.Content));
            Assert.Equal(2, messageInstance.Properties.Count);
            Assert.Equal("辉煌的混蛋", messageInstance.Properties["辉煌的混蛋"]);
            Assert.Equal("辉煌的混", messageInstance.Properties["辉煌的混"]);

            ///cleanup
        }
        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_008: [ If any parameter is null, constructor shall throw a ArgumentNullException ] */
        [Fact]
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
                Assert.Contains("contentAsByteArray cannot be null", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown."); 


            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_008: [ If any parameter is null, constructor shall throw a ArgumentNullException ] */
        [Fact]
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
                Assert.Contains("properties cannot be null", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown."); 


            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_004: [ Message class shall have a constructor that receives a content as byte[] and properties, storing them. ] */
        [Fact]
        public void Message_bytearrayAndPropertiesContructor_0bytes_0Properties_succeed()
        {
            ///arrage
            var inputByteArray = new byte[0];
            ///act
            var messageInstance = new Message(inputByteArray, new System.Collections.Generic.Dictionary<string, string>());

            Assert.Equal(inputByteArray, messageInstance.Content);
            Assert.Equal(0, messageInstance.Properties.Count);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_004: [ Message class shall have a constructor that receives a content as byte[] and properties, storing them. ] */
        [Fact]
        public void Message_bytearrayAndPropertiesContructor_1bytes_0Properties_succeed()
        {
            ///arrage
            var inputByteArray = new byte[1];
            inputByteArray[0] = (byte)'A';
            ///act
            var messageInstance = new Message(inputByteArray, new System.Collections.Generic.Dictionary<string, string>());

            Assert.Equal(inputByteArray, messageInstance.Content);
            Assert.Equal(inputByteArray[0], messageInstance.Content[0]);
            Assert.Equal(0, messageInstance.Properties.Count);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_004: [ Message class shall have a constructor that receives a content as byte[] and properties, storing them. ] */
        [Fact]
        public void Message_bytearrayAndPropertiesContructor_1bytes_1Properties_succeed()
        {
            ///arrage
            var inputByteArray = new byte[1];
            inputByteArray[0] = (byte)'A';
            var inputProperties = new System.Collections.Generic.Dictionary<string, string>();

            inputProperties.Add("Prop1", "Value1");

            ///act
            var messageInstance = new Message(inputByteArray, inputProperties);

            Assert.Equal(inputByteArray, messageInstance.Content);
            Assert.Equal(inputByteArray[0], messageInstance.Content[0]);
            Assert.Equal(1, messageInstance.Properties.Count);
            Assert.Equal("Value1", messageInstance.Properties["Prop1"]);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_004: [ Message class shall have a constructor that receives a content as byte[] and properties, storing them. ] */
        [Fact]
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

            Assert.Equal(inputByteArray, messageInstance.Content);
            Assert.Equal(inputByteArray[0], messageInstance.Content[0]);
            Assert.Equal(2, messageInstance.Properties.Count);
            Assert.Equal("Value1", messageInstance.Properties["Prop1"]);
            Assert.Equal("Value2", messageInstance.Properties["Prop2"]);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_005: [ Message Class shall have a ToByteArray method which will convert it's byte array Content and it's Properties to a byte[] which format is described at message_requirements.md ] */
        [Fact]
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

            Assert.Equal(14, byteArrayInstance.Length);
            Assert.True(StructuralComparisons.StructuralEqualityComparer.Equals(notFail____minimalMessage, byteArrayInstance));


            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_005: [ Message Class shall have a ToByteArray method which will convert it's byte array Content and it's Properties to a byte[] which format is described at message_requirements.md ] */
        [Fact]
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

            Assert.Equal(15, byteArrayInstance.Length);
            Assert.True(StructuralComparisons.StructuralEqualityComparer.Equals(notFail__0Property_1bytes, byteArrayInstance));


            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_005: [ Message Class shall have a ToByteArray method which will convert it's byte array Content and it's Properties to a byte[] which format is described at message_requirements.md ] */
        [Fact]
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

            Assert.Equal(16, byteArrayInstance.Length);
            Assert.True(StructuralComparisons.StructuralEqualityComparer.Equals(notFail__0Property_2bytes, byteArrayInstance));


            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_005: [ Message Class shall have a ToByteArray method which will convert it's byte array Content and it's Properties to a byte[] which format is described at message_requirements.md ] */
        [Fact]
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

            Assert.Equal(18, byteArrayInstance.Length);
            Assert.True(StructuralComparisons.StructuralEqualityComparer.Equals(notFail__1Property_0bytes, byteArrayInstance));


            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_005: [ Message Class shall have a ToByteArray method which will convert it's byte array Content and it's Properties to a byte[] which format is described at message_requirements.md ] */
        [Fact]
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

            Assert.Equal(44, byteArrayInstance.Length);
            Assert.True(StructuralComparisons.StructuralEqualityComparer.Equals(notFail__1Property_1bytes, byteArrayInstance));


            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_005: [ Message Class shall have a ToByteArray method which will convert it's byte array Content and it's Properties to a byte[] which format is described at message_requirements.md ] */
        [Fact]
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

            Assert.Equal(45, byteArrayInstance.Length);
            Assert.True(StructuralComparisons.StructuralEqualityComparer.Equals(notFail__1Property_2bytes, byteArrayInstance));


            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_005: [ Message Class shall have a ToByteArray method which will convert it's byte array Content and it's Properties to a byte[] which format is described at message_requirements.md ] */
        [Fact]
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

            Assert.Equal(23, byteArrayInstance.Length);
            Assert.True(StructuralComparisons.StructuralEqualityComparer.Equals(notFail__2Property_0bytes, byteArrayInstance));


            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_005: [ Message Class shall have a ToByteArray method which will convert it's byte array Content and it's Properties to a byte[] which format is described at message_requirements.md ] */
        [Fact]
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

            Assert.Equal(63, byteArrayInstance.Length);
            Assert.True(StructuralComparisons.StructuralEqualityComparer.Equals(notFail__2Property_1bytes, byteArrayInstance));


            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_005: [ Message Class shall have a ToByteArray method which will convert it's byte array Content and it's Properties to a byte[] which format is described at message_requirements.md ] */
        [Fact]
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

            Assert.Equal(64, byteArrayInstance.Length);
            Assert.True(StructuralComparisons.StructuralEqualityComparer.Equals(notFail__2Property_2bytes, byteArrayInstance));


            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_005: [ Message Class shall have a ToByteArray method which will convert it's byte array Content and it's Properties to a byte[] which format is described at message_requirements.md ] */
        [Fact]
        public void Message_ToByteArray_with_ChineseProperties_2bytes_happy_path()
        {
            ///arrage
            var inputProperties = new System.Collections.Generic.Dictionary<string, string>();

            inputProperties.Add("辉煌的混蛋", "辉煌的混蛋");
            inputProperties.Add("辉煌的混", "辉煌的混");
            var messageInstance = new Message("辉煌的混蛋", inputProperties);

            ///act
            var byteArrayInstance = messageInstance.ToByteArray();

            var messageToValidate = new Message(byteArrayInstance);

            Assert.Equal("辉煌的混蛋", System.Text.Encoding.UTF8.GetString(messageToValidate.Content));
            Assert.Equal(2, messageToValidate.Properties.Count);
            Assert.Equal("辉煌的混蛋", messageToValidate.Properties["辉煌的混蛋"]);
            Assert.Equal("辉煌的混", messageToValidate.Properties["辉煌的混"]);

            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_005: [ Message Class shall have a ToByteArray method which will convert it's byte array Content and it's Properties to a byte[] which format is described at message_requirements.md ] */
        [Fact]
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

            Assert.Equal(64, byteArrayInstance.Length);
            Assert.True(StructuralComparisons.StructuralEqualityComparer.Equals(notFail__2Property_2bytes, byteArrayInstance));


            ///cleanup
        }

        /* Tests_SRS_DOTNET_CORE_MESSAGE_04_005: [ Message Class shall have a ToByteArray method which will convert it's byte array Content and it's Properties to a byte[] which format is described at message_requirements.md ] */
        [Fact]
        public void Message_MessageConstructor_with_null_arg_Throw()
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
                Assert.Contains("message cannot be null", e.Message);
                return;
            }
            Assert.True(false, "No exception was thrown."); 


            ///cleanup
        }


        [Fact]
        public void Message_MessageConstructor_withValidMessage()
        {
            ///arrage

            var inputByteArray = new byte[1];
            inputByteArray[0] = (byte)'A';
            var inputProperties = new System.Collections.Generic.Dictionary<string, string>();

            inputProperties.Add("Prop1", "Value1");
            inputProperties.Add("Prop2", "Value2");
            Message secondMessage;

            {
                var messageInstance = new Message(inputByteArray, inputProperties);
                ///act

                secondMessage = new Message(messageInstance);
            }




            ///act
            Assert.Equal(inputByteArray, secondMessage.Content);
            Assert.Equal(inputByteArray[0], secondMessage.Content[0]);
            Assert.Equal(2, secondMessage.Properties.Count);
            Assert.Equal("Value1", secondMessage.Properties["Prop1"]);
            Assert.Equal("Value2", secondMessage.Properties["Prop2"]);
            ///cleanup
        }
    }
}
