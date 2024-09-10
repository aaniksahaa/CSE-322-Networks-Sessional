import com.sun.net.httpserver.SimpleFileServer;
import util.Config;

import java.net.InetSocketAddress;
import java.nio.file.Path;

public class Test2 {
    public static void main(String[] args) {
        SimpleFileServer.createFileServer(
                new InetSocketAddress(5001),
                Path.of(Config.rootPath),
                SimpleFileServer.OutputLevel.VERBOSE
        ).start();
    }
}