/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.moppy.core.comms.bridge;

import com.fazecast.jSerialComm.SerialPort;
import com.moppy.core.comms.MoppyMessage;
import com.moppy.core.comms.MoppyMessageFactory;
import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import java.util.function.Consumer;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * A Serial connection for Moppy devices.
 */
public class BridgeSerial extends NetworkBridge {

    private final SerialPort serialPort;
    private Thread listenerThread = null;
    
    public BridgeSerial(String serialPortName) {
        serialPort = SerialPort.getCommPort(serialPortName);
        serialPort.setBaudRate(57600);
    }
    
    @Override
    public void connect() throws IOException {
        if (!serialPort.openPort()) {
            throw new IOException("Failed to open serialPort!");
        }
        
        // Create and start listener thread
        SerialListener listener = new SerialListener(serialPort, this::messageToReceivers);
        listenerThread = new Thread(listener);
        listenerThread.start();
    }

    @Override
    public void sendMessage(MoppyMessage messageToSend) throws IOException {
        if (serialPort.isOpen()) {
            serialPort.writeBytes(messageToSend.getMessageBytes(), messageToSend.getMessageBytes().length);
        }
    }

    @Override
    public void close() throws IOException {
        serialPort.closePort();
        // Stop and cleanup listener thread
        listenerThread.interrupt();
        listenerThread = null;
    }
    
    /**
     * Listens to the serial port for MoppyMessages.  Because *all* this 
     * thread does is listen for messages, it's fine to block on serial.read().
     */
    private class SerialListener implements Runnable {

        private final SerialPort serialPort;
        private final Consumer<MoppyMessage> messageConsumer;
        
        public SerialListener(SerialPort serialPort, Consumer<MoppyMessage> messageConsumer) {
            this.serialPort = serialPort;
            this.messageConsumer = messageConsumer;
        }
        
        @Override
        public void run() {
            
            // MoppyMessages can't be longer than 259 bytes (SOM, DEVADDR, SUBADDR, LEN, [0-255 body bytes])
            // Longer messages will be truncated, but we don't care about them anyway
            byte[] buffer = new byte[259];
            buffer[0] = MoppyMessage.START_BYTE; // We'll be eating the start byte below, so make sure it's here
            int totalMessageLength = 1;
            
            try (InputStream serialIn = serialPort.getInputStream()) {
                while (serialPort.isOpen() && !Thread.interrupted()) {
                    // Keep reading until we get a START_BYTE
                    if (serialIn.read() == MoppyMessage.START_BYTE) {
                        buffer[1] = (byte)serialIn.read();
                        if (buffer[1] == MoppyMessage.SYSTEM_ADDRESS) {
                            buffer[2] = (byte)serialIn.read(); // Get body size
                            serialIn.read(buffer, 3, buffer[2]); // Read body into buffer
                            totalMessageLength = 3 + buffer[2];
                        } else {
                            buffer[2] = (byte)serialIn.read(); // Get Sub-Address
                            buffer[3] = (byte)serialIn.read(); // Get body size
                            serialIn.read(buffer, 4, buffer[3]); // Read body into buffer
                            totalMessageLength = 4 + buffer[3];
                        }
                        messageConsumer.accept(MoppyMessageFactory.fromBytes(Arrays.copyOf(buffer, totalMessageLength)));
                    }
                }
            } catch (IOException ex) {
                Logger.getLogger(BridgeSerial.class.getName()).log(Level.WARNING, null, ex);
            }
        }
        
    }
}