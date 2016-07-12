// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

using System;
using System.Collections.Generic;
using System.Collections;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Microsoft.Azure.IoT.Gateway
{
    /// <summary> Object that represents a message on the message bus. </summary>
    public class Message
    {
        /// <summary>
        ///   Message Content.
        /// </summary>
        public byte[] Content { get; }

        /// <summary>
        ///    Message Properties.
        /// </summary>
        public Dictionary<string, string> Properties { get; }

        private bool readNullTerminatedString(MemoryStream bis, out byte[] output)
        {
            List<byte> list = new List<byte>();
            bool result = false;
            output = null;

            byte b = (byte)bis.ReadByte();

            if (b != 255)
            {
                bool errorFound = false;
                while (b != '\0')
                {
                    list.Add(b);
                    b = (byte)bis.ReadByte();

                    if (b == 255)
                    {
                        errorFound = true;
                        break;
                    }
                }

                if (!errorFound)
                {
                    output = list.ToArray();
                    result = true;                        
                }
            };

            return result;
        }

        private int readIntFromMemoryStream(MemoryStream input)
        {
            byte[] byteArray = new byte[4];

            if(input.Read(byteArray, 0, 4) < 4)
            {
                throw new ArgumentException("Input doesn't have 4 bytes.");
            }

            if(BitConverter.IsLittleEndian)
            {
                Array.Reverse(byteArray); //Have to reverse because BitConverter expects a MSB 
            }            
            return BitConverter.ToInt32(byteArray, 0);
        }

        /// <summary>
        ///     Constructor for Message. This receives a byte array. Format defined at <a href="https://github.com/Azure/azure-iot-gateway-sdk/blob/master/core/devdoc/message_requirements.md">message_requirements.md</a>.
        /// </summary>
        /// <param name="msgAsByteArray">ByteArray with the Content and Properties of a message.</param>
        public Message(byte[] msgAsByteArray)
        {
            if (msgAsByteArray == null)
            {
                /* Codes_SRS_DOTNET_MESSAGE_04_008: [ If any parameter is null, constructor shall throw a ArgumentNullException ] */
                throw new ArgumentNullException("msgAsByteArray", "msgAsByteArray cannot be null");                    
            }
            /* Codes_SRS_DOTNET_MESSAGE_04_002: [ Message class shall have a constructor that receives a byte array with it's content format as described in message_requirements.md and it's Content and Properties are extracted and saved. ] */
            else if (msgAsByteArray.Length >= 14)
            {
                MemoryStream stream = new MemoryStream(msgAsByteArray);
                this.Properties = new Dictionary<string, string>();

                byte header1 = (byte)stream.ReadByte();
                byte header2 = (byte)stream.ReadByte();

                if (header1 == (byte)0xA1 && header2 == (byte)0x60)
                {
                    int arraySizeInInt;
                    try
                    {
                        arraySizeInInt = readIntFromMemoryStream(stream);
                    }
                    catch(ArgumentException e)
                    {
                        /* Codes_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
                        throw new ArgumentException("Could not read array size information.", e);
                    }

                    if (arraySizeInInt >= int.MaxValue)
                    {
                        /* Codes_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
                        throw new ArgumentException("Size of MsgArray can't be more than MAXINT.");
                    }

                    if (msgAsByteArray.Length != arraySizeInInt)
                    {
                        /* Codes_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
                        throw new ArgumentException("Array Size information doesn't match with array size.");
                    }

                    int propCount;

                    try
                    {
                        propCount = readIntFromMemoryStream(stream);
                    }
                    catch (ArgumentException e)
                    {
                        /* Codes_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
                        throw new ArgumentException("Could not read property count.", e);
                    }

                    if(propCount < 0)
                    {
                        /* Codes_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
                        throw new ArgumentException("Number of properties can't be negative."); 
                    }

                    if (propCount >= int.MaxValue)
                    {
                        /* Codes_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
                        throw new ArgumentException("Number of properties can't be more than MAXINT.");
                    }

                    if (propCount > 0)
                    {
                        //Here is where we are going to read the properties.
                        for (int count = 0; count < propCount; count++)
                        {
                            byte[] key;
                            if(!readNullTerminatedString(stream, out key))
                            {
                                /* Codes_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
                                throw new ArgumentException("Could not parse Properties(key)");
                            }

                            byte[] value;
                            if (!readNullTerminatedString(stream, out value))
                            {
                                /* Codes_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
                                throw new ArgumentException("Could not parse Properties(value)");
                            }

                            this.Properties.Add(System.Text.Encoding.UTF8.GetString(key, 0, key.Length), System.Text.Encoding.UTF8.GetString(value, 0, value.Length));
                        }
                    }

                    long contentLengthPosition = stream.Position;
                    int contentLength;

                    try
                    {
                        contentLength = readIntFromMemoryStream(stream);
                    }
                    catch (ArgumentException e)
                    {
                        /* Codes_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
                        throw new ArgumentException("Could not read contentLength.", e);
                    }
                    

                    // Verify if the number of content matches with the real number of content. 
                    // 4 is the number of bytes that describes the contentLengthPosition information.
                    if (arraySizeInInt - contentLengthPosition - 4 != contentLength)
                    {
                        /* Codes_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
                        throw new ArgumentException("Size of byte array doesn't match with current content.");
                    }

                    byte[] content = new byte[contentLength];
                    stream.Read(content, 0, contentLength);

                    this.Content = content;
                }
                else
                {
                    /* Codes_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
                    throw new ArgumentException("Invalid Header bytes.");
                }
            }
            else
            {
                /* Codes_SRS_DOTNET_MESSAGE_04_006: [ If byte array received as a parameter to the Message(byte[] msgInByteArray) constructor is not in a valid format, it shall throw an ArgumentException ] */
                throw new ArgumentException("Invalid byte array size.");
            }
        }

        /// <summary>
        ///     Constructor for Message. This constructor receives a byte[] as it's content and Properties.
        /// </summary>
        /// <param name="contentAsByteArray">Content of the Message</param>
        /// <param name="properties">Set of Properties that will be added to a message.</param>
        public Message(byte[] contentAsByteArray, Dictionary<string, string> properties)
        {
            if (contentAsByteArray == null)
            {
                /* Codes_SRS_DOTNET_MESSAGE_04_008: [ If any parameter is null, constructor shall throw a ArgumentNullException ] */
                throw new ArgumentNullException("contentAsByteArray", "contentAsByteArray cannot be null");
            }
            else if(properties == null)
            {
                /* Codes_SRS_DOTNET_MESSAGE_04_008: [ If any parameter is null, constructor shall throw a ArgumentNullException ] */
                throw new ArgumentNullException("properties", "properties cannot be null");
            }
            else
            {
                /* Codes_SRS_DOTNET_MESSAGE_04_004: [ Message class shall have a constructor that receives a content as byte[] and properties, storing them. ] */
                this.Content = contentAsByteArray;
                this.Properties = properties;
            }
        }

        /// <summary>
        ///     Constructor for Message. This constructor receives a string as it's content and Properties.
        /// </summary>
        /// <param name="content">String with the ByteArray with the Content and Properties of a message.</param>
        /// <param name="properties">Set of Properties that will be added to a message.</param>
        public Message(string content, Dictionary<string, string> properties)
        {
            
            if (content == null)
            {
                /* Codes_SRS_DOTNET_MESSAGE_04_008: [ If any parameter is null, constructor shall throw a ArgumentNullException ] */
                throw new ArgumentNullException("content", "content cannot be null");
            }
            else if (properties == null)
            {
                /* Codes_SRS_DOTNET_MESSAGE_04_008: [ If any parameter is null, constructor shall throw a ArgumentNullException ] */
                throw new ArgumentNullException("properties", "properties cannot be null");
            }
            else
            {
                /* Codes_SRS_DOTNET_MESSAGE_04_003: [ Message class shall have a constructor that receives a content as string and properties and store it. This string shall be converted to byte array based on System.Text.Encoding.UTF8.GetBytes(). ] */
                this.Content = System.Text.Encoding.UTF8.GetBytes(content);
                this.Properties = properties;
            }
            
        }

        /// <summary>
        ///     Constructor for Message. This constructor receives another Message as a parameter.
        /// </summary>
        /// <param name="message">Message Instance.</param>
        public Message(Message message)
        {
            if(message == null)
            {
                /* Codes_SRS_DOTNET_MESSAGE_04_008: [ If any parameter is null, constructor shall throw a ArgumentNullException ] */
                throw new ArgumentNullException("message","message cannot be null");
            }
            else
            {
                this.Content = message.Content;
                this.Properties = message.Properties;
            }
        }


        private long getPropertiesByteAmount()
        {
            long sizeOfPropertiesInBytes = 0;
            foreach (KeyValuePair<string, string> propertyItem in this.Properties)
            {

                sizeOfPropertiesInBytes += System.Text.Encoding.UTF8.GetByteCount(propertyItem.Key) + 1;
                sizeOfPropertiesInBytes += System.Text.Encoding.UTF8.GetByteCount(propertyItem.Value) + 1;
            }

            return sizeOfPropertiesInBytes;
        }

        private long fillByteArrayWithPropertyInBytes(byte[] dst)
        {
            //The content needs to be filled from byte 11th. 
            long currentIndex = 10;

            foreach (KeyValuePair<string, string> propertiItem in this.Properties)
            {
                byte[] bytesOfKey = System.Text.Encoding.UTF8.GetBytes(propertiItem.Key);
                
                foreach(byte byteItem in bytesOfKey)
                {
                    dst[currentIndex++] = byteItem;
                }

                dst[currentIndex++] = 0;

                byte[] bytesOfValue = System.Text.Encoding.UTF8.GetBytes(propertiItem.Value);

                foreach (byte byteItem in bytesOfValue)
                {
                    dst[currentIndex++] = byteItem;
                }

                dst[currentIndex++] = 0;
            }

            return currentIndex;
        }

        /// <summary>
        ///    Converts the message into a byte array (Format defined at <a href="https://github.com/Azure/azure-iot-gateway-sdk/blob/master/core/devdoc/message_requirements.md">message_requirements.md</a>).
        /// </summary>
        virtual public byte[] ToByteArray()
        {
            /* Codes_SRS_DOTNET_MESSAGE_04_005: [ Message Class shall have a ToByteArray method which will convert it's byte array Content and it's Properties to a byte[] which format is described at message_requirements.md ] */
            //1-Calculate the size of the array;
            long sizeOfArray = 2 + 4 + 4 + getPropertiesByteAmount() + 4 + this.Content.Length;

            //2-Create the byte array;
            byte[] returnByteArray = new Byte[sizeOfArray];

            //3-Fill the first 2 bytes with 0xA1 and 0x60
            returnByteArray[0] = 0xA1;
            returnByteArray[1] = 0x60;

            //4-Fill the 4 bytes with the array size;
            byte[] sizeOfArrayByteArray = BitConverter.GetBytes(sizeOfArray); //This will get an 8 bytes, since sizeOfArray is a long. Ignoring the most significant bit.
            Array.Reverse(sizeOfArrayByteArray); //Have to reverse because this is not MSB and needs to be.
            returnByteArray[2] = sizeOfArrayByteArray[4];
            returnByteArray[3] = sizeOfArrayByteArray[5];
            returnByteArray[4] = sizeOfArrayByteArray[6];
            returnByteArray[5] = sizeOfArrayByteArray[7];

            //5-Fill the 4 bytes with the amount of properties;
            byte[] numberOfPropertiesInByteArray = BitConverter.GetBytes(this.Properties.Count);
            Array.Reverse(numberOfPropertiesInByteArray); //Have to reverse because this is not MSB and needs to be. 
            returnByteArray[6] = numberOfPropertiesInByteArray[0];
            returnByteArray[7] = numberOfPropertiesInByteArray[1];
            returnByteArray[8] = numberOfPropertiesInByteArray[2];
            returnByteArray[9] = numberOfPropertiesInByteArray[3];

            //6-Fill the bytes with content from key/value of properties (null terminated string separated);
            long msgContentShallStartFromHere = fillByteArrayWithPropertyInBytes(returnByteArray);

            //7-Fill the amount of bytes on the content in 4 bytes after the properties; 
            byte[] contentSizeInByteArray = BitConverter.GetBytes(this.Content.Length);
            Array.Reverse(contentSizeInByteArray); //Have to reverse because this is not MSB and needs to be. 
            returnByteArray[msgContentShallStartFromHere++] = contentSizeInByteArray[0];
            returnByteArray[msgContentShallStartFromHere++] = contentSizeInByteArray[1];
            returnByteArray[msgContentShallStartFromHere++] = contentSizeInByteArray[2];
            returnByteArray[msgContentShallStartFromHere++] = contentSizeInByteArray[3];

            //8-Fill up the bytes with the message content. 

            foreach (byte contentElement in this.Content)
            {
                returnByteArray[msgContentShallStartFromHere++] = contentElement;
            }


            return returnByteArray;
        }
    }
}
