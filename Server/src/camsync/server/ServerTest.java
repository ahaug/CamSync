package camsync.server;

import java.net.InetAddress;
import java.net.Socket;
import java.util.Random;

/**
 *
 * @author jpettit
 */
public class ServerTest extends Thread {

    Random rand = new Random();

    public ServerTest() {
    }

    @Override
    public void run() {
        try {
            Thread.sleep(1000 + rand.nextInt(5000));
            Messages.Msg.Image.Builder builder = Messages.Msg.Image.newBuilder();
            Messages.Msg.Image image = builder.build();
            Messages.Msg.Builder builder1 = Messages.Msg.newBuilder();
            builder1.setImage(image);
            Socket sock = new Socket(InetAddress.getLocalHost(), 5080);
            Messages.Msg msg = builder1.build();
            msg.writeDelimitedTo(sock.getOutputStream());
        } catch (Exception ex) {
            ex.printStackTrace();
            return;
        }
    }
}
