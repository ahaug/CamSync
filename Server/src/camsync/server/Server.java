package camsync.server;

import java.net.ServerSocket;
import java.net.Socket;
import java.util.LinkedList;
import java.util.List;
import joptsimple.OptionParser;
import joptsimple.OptionSet;

/**
 *
 * @author jpettit
 */
public class Server extends Thread {

    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) throws Exception {
        OptionParser parser = new OptionParser();
        parser.accepts("h", "Print help");

        OptionSet options = parser.parse(args);

        if (options.has("h")) {
            parser.printHelpOn(System.out);
            System.exit(0);
        }
        new Server().start();
    }

    private List<ClientHandler> clients;

    public Server() {
        clients = new LinkedList<ClientHandler>();
    }

    public void getPictures() {
        for (ClientHandler client : clients) {
            client.getPicture();
        }
    }

    @Override
    public void run() {
        System.err.println("starting server");
        try {
            ServerSocket sock = new ServerSocket(5080);
            Socket client;
            while ((client = sock.accept()) != null) {
                ClientHandler handler = new ClientHandler(client);
                handler.start();
                clients.add(handler);
                if (clients.size() == 1) {
                    getPictures();
                }
            }
        } catch (Exception ex) {
            ex.printStackTrace();
            System.exit(1);
        }
    }
}
