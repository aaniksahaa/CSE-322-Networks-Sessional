package server;

import util.Config;
import util.Helper;
import util.LogWriter;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;

public class Server {
    public static void main(String[] args) throws IOException, ClassNotFoundException {
        Helper.initializeUploadDir();
        ServerSocket serverSocket = new ServerSocket(Config.SERVER_PORT);
        System.out.println("Server started.\nListening for connections on port : " + Config.SERVER_PORT + " ...\n");
        LogWriter logWriter = new LogWriter("server_log.txt");

        while(true) {
            Socket socket = serverSocket.accept();
            Thread worker = new Worker(socket, logWriter);
            worker.start();
        }
    }
}
