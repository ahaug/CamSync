package camsync.server;

import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
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
        parser.accepts("t", "Test server");

        OptionSet options = parser.parse(args);

        if (options.has("h")) {
            parser.printHelpOn(System.out);
            System.exit(0);
        } else if (options.has("t")) {
            for (int i = 0; i < 10; i++) {
                ServerTest test = new ServerTest();
                test.start();
            }
        }
        new Server().start();
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
            }
        } catch (Exception ex) {
            ex.printStackTrace();
            System.exit(1);
        }
    }
}
