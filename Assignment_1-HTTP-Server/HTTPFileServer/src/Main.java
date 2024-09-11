import FileSystem.FileSystemNode;
import util.Config;
import util.HtmlGenerator;
import util.HttpStatus;

import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Date;

import static util.Config.*;

public class Main {

    public static void writeHeaderPrimer(PrintWriter pr, HttpStatus httpStatus) {
        pr.write(Config.HTTP_VERSION + " " + httpStatus + "\r\n");
        pr.write("Server: " + Config.SERVER_NAME + "\r\n");
        pr.write("Date: " + new Date() + "\r\n");
    }

    public static void writeHeaderEnd(PrintWriter pr) {
        pr.write("\r\n");
    }

    public static void sendStatusResponse(PrintWriter pr, HttpStatus httpStatus) throws IOException {
        String content = "<html><body><h1>" + httpStatus + "</h1></body></html>";

        writeHeaderPrimer(pr, httpStatus);
        pr.write("Content-Type: text/html\r\n");
        pr.write("Content-Length: " + content.length() + "\r\n");

        writeHeaderEnd(pr);

        pr.write(content);
        pr.flush();
    }

    private static void sendFile(PrintWriter pr, OutputStream os, FileSystemNode node) throws IOException {
        File file = node.file;

        writeHeaderPrimer(pr,OK);
        pr.write("Content-Type: " + node.contentType + "\r\n");
        pr.write("Content-Length: " + file.length() + "\r\n");
        if(!node.isTextOrImage()){
            pr.write("Content-Disposition: attachment; filename=\"" + file.getName() + "\"\r\n");
        }
        writeHeaderEnd(pr);
        pr.flush();

        try (FileInputStream fis = new FileInputStream(file)) {
            byte[] buffer = new byte[Config.CHUNK_SIZE];
            int bytesRead;
            while ((bytesRead = fis.read(buffer)) != -1) {
                os.write(buffer, 0, bytesRead);
            }
            os.flush();
        } catch (Exception e){
            e.printStackTrace();
        }
    }

    public static void sendStringContent(PrintWriter pr, String content, String contentType) {
        writeHeaderPrimer(pr,OK);
        pr.write("Content-Type: " + contentType + "\r\n");
        pr.write("Content-Length: " + content.length() + "\r\n");
        writeHeaderEnd(pr);

        pr.write(content);
        pr.flush();
    }

    public static void sendDirectoryListing(PrintWriter pr, FileSystemNode node) throws IOException{
        String htmlContent = HtmlGenerator.generateDirectoryListingHtml(node);
        sendStringContent(pr,htmlContent,"text/html");
    }

    private static boolean isUploadable(String contentType) {
        if (contentType.startsWith("text/") || contentType.startsWith("image/") || contentType.startsWith("video/")) {
            return true;
        }
        return false;
    }

    public static void main(String[] args) throws IOException {
        ServerSocket serverSocket = new ServerSocket(Config.SERVER_PORT);
        System.out.println("Server started.\nListening for connections on port : " + Config.SERVER_PORT + " ...\n");

        while(true)
        {
            Socket s = serverSocket.accept();
            System.out.println("New connection established...");

            OutputStream os = s.getOutputStream();
            InputStream is = s.getInputStream();

            BufferedReader in = new BufferedReader(new InputStreamReader(is));
            PrintWriter pr = new PrintWriter(os);

            String requestLine = in.readLine();

            if(requestLine == null) continue;
            if(requestLine.length() > 0) {
                System.out.println(requestLine);

                if(requestLine.startsWith("GET"))
                {
                    String[] parts = requestLine.split(" ", 3);

                    if(parts.length < 2){
                        sendStatusResponse(pr, BAD_REQUEST);
                        continue;
                    }

                    String method = parts[0];
                    String path = parts[1];

                    FileSystemNode node = new FileSystemNode(path);

                    if(node.isValid){
                        if(node.isDirectory){
                            sendDirectoryListing(pr, node);
                        } else {
                            sendFile(pr, os, node);
                        }
                    } else {
                        sendStatusResponse(pr, NOT_FOUND);
                    }
                }
                else if(requestLine.startsWith("UPLOAD"))
                {
                    String[] parts = requestLine.split(" ", 2);

                    if(parts.length != 2){
                        sendStatusResponse(pr, BAD_REQUEST);
                        continue;
                    }

                    String fileName = parts[1];

                    String contentTypeLine = in.readLine();
                    String contentType = contentTypeLine.split(": ")[1];

                    if(! isUploadable(contentType)){
                        String msg = UPLOAD_FILE_FORMAT_ERROR;
                        System.out.println(msg);
                        sendStatusResponse(pr, new HttpStatus(400, msg));
                        continue;
                    }

                    String contentLengthLine = in.readLine();
                    long fileSize = Long.parseLong(contentLengthLine.split(": ")[1]);

                    // skipping the extra /r/n
                    // denoting the end of headers
                    in.readLine();

                    Boolean success = saveFile(is, fileName, fileSize);

                    if(success){
                        sendStatusResponse(pr, new HttpStatus(200, "File uploaded successfully"));
                    } else {
                        sendStatusResponse(pr, new HttpStatus(500, "Server Error. File could not be uploaded"));
                    }
                    pr.write(Config.HTTP_VERSION + " 200 " + "File Uploaded Successfully" + "\r\n");

                    System.out.println("File uploaded successfully: " + fileName);
                }
                else
                {
                    sendStatusResponse(pr, BAD_REQUEST);
                }
            }

            s.close();
        }
    }

    private static Boolean saveFile(InputStream is, String fileName, long fileSize) {
        File file = new File("root/uploaded", fileName);

        try (FileOutputStream fos = new FileOutputStream(file)) {
            byte[] buffer = new byte[Config.CHUNK_SIZE];
            int bytesRead;
            long totalBytesRead = 0;

            while (totalBytesRead < fileSize && (bytesRead = is.read(buffer)) != -1) {
                fos.write(buffer, 0, bytesRead);
                totalBytesRead += bytesRead;
            }

            System.out.println("File saved to: " + file.getAbsolutePath());
            return true;

        } catch (IOException e) {
            System.err.println("Error saving file: " + e.getMessage());
            return false;
        }
    }

}
