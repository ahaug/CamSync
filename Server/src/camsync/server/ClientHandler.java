package camsync.server;

import java.net.Socket;

/**
 *
 * @author jpettit
 */
public class ClientHandler extends Thread {

    private Socket client;

    public ClientHandler(Socket socket) {
        client = socket;
    }

    @Override
    public void run() {
        System.err.println("starting client handler");
        try {
            Messages.Msg msg = Messages.Msg.parseDelimitedFrom(client.getInputStream());
            if (msg.hasImage()) {
                System.err.println("received image from client");
            }
            client.close();
        } catch (Exception ex) {
            ex.printStackTrace();
            return;
        }
        System.err.println("tearing down client handler");
    }
}
