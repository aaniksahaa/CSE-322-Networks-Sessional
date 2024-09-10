import FileSystem.FileSystemNode;
import util.Config;
import util.HtmlGenerator;
import util.Status;

import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.file.Files;
import java.util.Date;

public class Main {
    static final int PORT = Config.SERVER_PORT;

    public static String readFileData(File file, int fileLength) throws IOException {
        FileInputStream fileIn = null;
        byte[] fileData = new byte[fileLength];
        try {
            fileIn = new FileInputStream(file);
            fileIn.read(fileData);
        } finally {
            if (fileIn != null)
                fileIn.close();
        }
        return String.valueOf(fileData);
    }

    public static void sendError(PrintWriter pr, Status status) throws IOException {
        String content = "<html><body><h1>" + status + "</h1></body></html>";

        pr.write(Config.HTTP_VERSION + " " + status);
        pr.write("Server: " + Config.SERVER_NAME + "\r\n");
        pr.write("Date: " + new Date() + "\r\n");
        pr.write("Content-Type: text/html\r\n");
        pr.write("Content-Length: " + content.length() + "\r\n");
        pr.write("\r\n");
        pr.write(content);
        pr.flush();
    }

    private static void sendFile(PrintWriter pr, OutputStream out, File file) throws IOException {
        String contentType = Files.probeContentType(file.toPath());
        if (contentType == null) {
            contentType = "application/octet-stream";
        }

        // Write headers using PrintWriter
        if (contentType.startsWith("text/") || contentType.startsWith("image/")) {
            pr.write("HTTP/1.0 200 OK\r\n");
            pr.write("Content-Type: " + contentType + "\r\n");
            pr.write("Content-Length: " + file.length() + "\r\n\r\n");
            pr.flush();  // Make sure all headers are sent

            System.out.println(file.length());
            System.out.println("Here");

            // Send file content using OutputStream
            try (FileInputStream fis = new FileInputStream(file)) {
                byte[] buffer = new byte[Config.CHUNK_SIZE];
                int bytesRead;
                while ((bytesRead = fis.read(buffer)) != -1) {
                    System.out.println(bytesRead);
                    out.write(buffer, 0, bytesRead);
                }
                out.flush();
            }
        } else {
            pr.write("HTTP/1.0 200 OK\r\n");
            pr.write("Content-Type: " + contentType + "\r\n");
            pr.write("Content-Length: " + file.length() + "\r\n");
            pr.write("Content-Disposition: attachment; filename=\"" + file.getName() + "\"\r\n\r\n");
            pr.flush();  // Flush the headers

            // Send file content using OutputStream
            try (FileInputStream fis = new FileInputStream(file)) {
                byte[] buffer = new byte[Config.CHUNK_SIZE];
                int bytesRead;
                while ((bytesRead = fis.read(buffer)) != -1) {
                    out.write(buffer, 0, bytesRead);
                }
                out.flush();
            }
        }
    }



    public static void main(String[] args) throws IOException {
        ServerSocket serverSocket = new ServerSocket(PORT);
        System.out.println("Server started.\nListening for connections on port : " + PORT + " ...\n");

        File file = new File("index.html");
        FileInputStream fis = new FileInputStream(file);
        BufferedReader br = new BufferedReader(new InputStreamReader(fis, "UTF-8"));

        StringBuilder sb = new StringBuilder();
        String line;
        while(( line = br.readLine()) != null ) {
            sb.append( line );
            sb.append( '\n' );
        }

//        String content = sb.toString();

        while(true)
        {
            Socket s = serverSocket.accept();
            BufferedReader in = new BufferedReader(new InputStreamReader(s.getInputStream()));
            PrintWriter pr = new PrintWriter(s.getOutputStream());

            String requestLine = in.readLine();
            System.out.println("input : "+requestLine);

            // String content = "<html>Hello</html>";
            if(requestLine == null) continue;
            if(requestLine.length() > 0) {
                String[] parts = requestLine.split(" ");

                String method = parts[0];
                String path = parts[1];
                String httpVersion = parts[2];

                if(method.equals("GET"))
                {
                    System.out.println(requestLine);

                    FileSystemNode node = new FileSystemNode(path);

                    if(!node.isValid){
                        sendError(pr, Config.NOT_FOUND);
                        continue;
                    }
                    if(!node.isDirectory){
                        sendFile(pr, s.getOutputStream(),node.file);
                        continue;
                    }

                    String content = HtmlGenerator.generateHtml(path);
                    pr.write("HTTP/1.1 200 OK\r\n");
                    pr.write("Server: Java HTTP Server: 1.0\r\n");
                    pr.write("Date: " + new Date() + "\r\n");
                    pr.write("Content-Type: text/html\r\n");
                    pr.write("Content-Length: " + content.length() + "\r\n");
                    pr.write("\r\n");
                    pr.write(content);
                    pr.flush();
                }
                else if(method.equals("UPLOAD"))
                {

                }
                else
                {
                    sendError(pr, Config.BAD_REQUEST);
                }
            }

            s.close();
        }

    }

}
