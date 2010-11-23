package camsync.server;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.Socket;

/**
 *
 * @author jpettit
 */
public class ClientHandler extends Thread {

    private Socket client;
    private boolean running;

    public ClientHandler(Socket socket) {
        client = socket;
    }

    public void getPicture() {
        System.err.println("getting picture from client");
        try {
            BufferedWriter out = new BufferedWriter(new OutputStreamWriter(client.getOutputStream()));
            out.write("take a picture");
            out.newLine();
            out.flush();
            BufferedReader in = new BufferedReader(new InputStreamReader(client.getInputStream()));
            System.err.println(in.readLine());
        } catch (IOException ex) {
            ex.printStackTrace();
        }
    }

    @Override
    public void run() {
        System.err.println("starting client handler");
        running = true;
        while (running) {
        }
        System.err.println("tearing down client handler");
    }

    public void close() {
        running = false;
    }
}
