package server;

import util.Config;
import util.LogWriter;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;

public class Server {
    public static void main(String[] args) throws IOException, ClassNotFoundException {
        ServerSocket serverSocket = new ServerSocket(Config.SERVER_PORT);
        System.out.println("Server started.\nListening for connections on port : " + Config.SERVER_PORT + " ...\n");
        LogWriter logWriter = new LogWriter("server_log.txt");

        while(true) {
            Socket socket = serverSocket.accept();
            System.out.println("\nNew Socket Connection established...\n");
            Thread worker = new Worker(socket, logWriter);
            worker.start();
        }
    }
}
