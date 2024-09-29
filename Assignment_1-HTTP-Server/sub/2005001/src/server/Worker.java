package server;

import util.*;

import java.io.*;
import java.net.Socket;
import java.util.Date;

import static util.Config.*;
import static util.Config.BAD_REQUEST;

public class Worker extends Thread {
    Socket socket;
    OutputStream os;
    InputStream is;
    BufferedReader in;
    PrintWriter pr;
    String requestLine;
    StringBuilder responseHeaderBuilder = new StringBuilder();
    LogWriter logWriter;

    public Worker(Socket socket, LogWriter logWriter) throws IOException {
        this.socket = socket;
        this.os = socket.getOutputStream();
        this.is = socket.getInputStream();
        this.in = new BufferedReader(new InputStreamReader(this.is));
        this.pr = new PrintWriter(this.os);
        this.logWriter = logWriter;
    }

    public void run()
    {
        try {
            String requestLine = in.readLine();
            this.requestLine = requestLine;

            if (requestLine == null) return;
            if (requestLine.length() > 0) {
                System.out.println("\n"+requestLine);
                System.out.println("Serving request in a new Thread...\n");

                if (requestLine.startsWith("GET")) {
                    String[] parts = requestLine.split(" ", 3);

                    if (parts.length < 2) {
                        sendStatusResponse(BAD_REQUEST);
                        clearArtefacts();
                        return;
                    }

                    String method = parts[0];
                    String path = parts[1];

                    FileSystemNode node = new FileSystemNode(path);

                    if (node.isValid) {
                        if (node.isDirectory) {
                            String htmlContent = HtmlGenerator.generateDirectoryListingHtml(node);
                            sendStringContent(htmlContent,"text/html");
                        } else if (node.isText()) {
                            String htmlContent = HtmlGenerator.generateTextHtml(node);
                            sendStringContent(htmlContent,"text/html");
                        } else if (node.isImage()) {
                            String htmlContent = HtmlGenerator.generateImageHtml(node);
                            sendStringContent(htmlContent,"text/html");
                        } else {
                            // enforce download if not text or image
                            sendFileAsAttachment(node);
                        }
                    } else {
                        System.out.println("Error 404: Directory or File not found...");
                        sendStatusResponse(NOT_FOUND);
                    }
                } else if (requestLine.startsWith("UPLOAD")) {
                    String[] parts = requestLine.split(" ", 2);

                    if (parts.length != 2) {
                        sendStatusResponse(BAD_REQUEST);
                        clearArtefacts();
                        return;
                    }

                    String fileName = parts[1];

                    String contentTypeLine = in.readLine();
                    String contentType = contentTypeLine.split(": ")[1];

                    if (!isUploadable(contentType)) {
                        String msg = UPLOAD_FILE_FORMAT_ERROR;
                        System.out.println("Error: "+msg);
                        sendStatusResponse(new HttpStatus(400, msg));
                        clearArtefacts();
                        return;
                    }

                    String contentLengthLine = in.readLine();
                    long fileSize = Long.parseLong(contentLengthLine.split(": ")[1]);

                    // skipping the extra /r/n
                    // denoting the end of headers
                    in.readLine();

                    Boolean success = saveFile(fileName, fileSize);

                    HttpStatus httpStatus;

                    if (success) {
                        httpStatus = new HttpStatus(200, "File uploaded successfully");
                    } else {
                        httpStatus = new HttpStatus(500, "Server Error. File could not be uploaded");
                    }

                    sendStatusResponse(httpStatus);
                    System.out.println(httpStatus.message);
                } else {
                    sendStatusResponse(BAD_REQUEST);
                }
            }

            clearArtefacts();
        } catch (Exception e){
            e.printStackTrace();
        }
    }
    public void clearArtefacts() throws IOException, InterruptedException {
        Thread.sleep(1000);
        socket.close();
        System.out.println("\nServed request " + requestLine + "");
        System.out.println("Socket connection closed\n");
    }
    public void writeHeaderPrimer(HttpStatus httpStatus) {
        responseHeaderBuilder.append(Config.HTTP_VERSION + " " + httpStatus + "\r\n");
        responseHeaderBuilder.append("Server: " + Config.SERVER_NAME + "\r\n");
        responseHeaderBuilder.append("Date: " + new Date() + "\r\n");
    }
    public void writeHeaderEnd() {
        responseHeaderBuilder.append("\r\n");
    }

    public void sendResponseHeader(){
        String responseHeader = responseHeaderBuilder.toString();
        pr.write(responseHeaderBuilder.toString());
        pr.flush();

        StringBuilder logBuilder = new StringBuilder();
        logBuilder.append("Request:\n"+requestLine+"\n\n");
        logBuilder.append("Response Header:\n"+responseHeader+"\n");
        logWriter.writeLog(logBuilder.toString());
    }

    public void sendStatusResponse(HttpStatus httpStatus) throws IOException {
        String content = "<html><body><h1>" + httpStatus + "</h1></body></html>";

        writeHeaderPrimer(httpStatus);
        responseHeaderBuilder.append("Content-Type: text/html\r\n");
        responseHeaderBuilder.append("Content-Length: " + content.length() + "\r\n");
        writeHeaderEnd();
        sendResponseHeader();

        pr.write(content);
        pr.flush();
    }

    private void sendFileAsAttachment(FileSystemNode node) throws IOException {
        File file = node.file;

        writeHeaderPrimer(OK);
        responseHeaderBuilder.append("Content-Type: " + node.contentType + "\r\n");
        responseHeaderBuilder.append("Content-Length: " + file.length() + "\r\n");

        // send as attachment to enforce download
        responseHeaderBuilder.append("Content-Disposition: attachment; filename=\"" + file.getName() + "\"\r\n");

        writeHeaderEnd();
        sendResponseHeader();

        try (FileInputStream fis = new FileInputStream(file)) {
            byte[] buffer = new byte[Config.CHUNK_SIZE];
            int bytesRead;
            while ((bytesRead = fis.read(buffer)) != -1) {
                os.write(buffer, 0, bytesRead);
                Thread.sleep(ARTIFICAL_SENDING_DELAY_MS);
            }
            os.flush();
        } catch (Exception e){
            e.printStackTrace();
        }
    }

    public void sendStringContent(String content, String contentType) {
        writeHeaderPrimer(OK);
        responseHeaderBuilder.append("Content-Type: " + contentType + "\r\n");
        responseHeaderBuilder.append("Content-Length: " + content.length() + "\r\n");
        writeHeaderEnd();
        sendResponseHeader();

        pr.write(content);
        pr.flush();
    }

    private boolean isUploadable(String contentType) {
        if (contentType.startsWith("text/") || contentType.startsWith("image/") || contentType.startsWith("video/")) {
            return true;
        }
        return false;
    }

    private Boolean saveFile(String fileName, long fileSize) {
        File file = new File(uploadDir, fileName);

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
