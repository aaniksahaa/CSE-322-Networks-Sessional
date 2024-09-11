package client;
import util.Config;
import util.Helper;

import java.io.*;
import java.net.Socket;
import java.util.Scanner;

public class Client {
    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);

        while (true) {
            System.out.println("Enter the file path to upload (or 'exit' to quit): ");
            String filePath = scanner.nextLine();

            if (filePath.equalsIgnoreCase("exit")) {
                break;
            }

            File file = new File(filePath);
            if (!file.exists()) {
                System.out.println("Sorry! The file does not exist.");
            } else {
                new Thread(() -> uploadFileToServer(file)).start();
            }
        }

        scanner.close();
    }

    private static void uploadFileToServer(File file) {
        final boolean[] stopSending = {false}; // Flag to stop sending data

        try (Socket socket = new Socket(Config.SERVER_HOST, Config.SERVER_PORT);
             FileInputStream fis = new FileInputStream(file);
             OutputStream os = socket.getOutputStream();
             InputStream is = socket.getInputStream()) {

            System.out.println("Connection established at port: " + socket.getPort());

            BufferedReader in = new BufferedReader(new InputStreamReader(is));
            PrintWriter pr = new PrintWriter(os);

            String contentType = Helper.getContentType(file);

            pr.write("UPLOAD " + file.getName() + "\r\n");
            pr.write("Content-Type: " + contentType + "\r\n");
            pr.write("Content-Length: " + file.length() + "\r\n\r\n");
            pr.flush();

            Thread readThread = new Thread(() -> {
                try {
                    String responseLine;
                    responseLine = in.readLine();
                    if(responseLine != null && responseLine.length() > 0){
                        String[] parts = responseLine.split(" ",3);
                        if(parts.length >= 3){
                            int statusCode = Integer.parseInt(parts[1]);
                            String statusMsg = parts[2];
                            if(statusCode != 200){
                                System.out.println("\nError: " + statusMsg);
                                stopSending[0] = true;
                            }
                        }
                    }
//                    while ((responseLine = in.readLine()) != null) {
//                        System.out.println("Server response: " + responseLine);
//                        if (responseLine.contains("Bad")) {
//                            System.out.println("Error from server: " + responseLine);
//                            stopSending[0] = true;
//                            break;
//                        }
//                    }
                } catch (IOException e) {
                    System.err.println("Error reading server response: " + e.getMessage());
                }
            });

            readThread.start();

            byte[] buffer = new byte[Config.CHUNK_SIZE];
            int bytesRead;
            while (!stopSending[0] && (bytesRead = fis.read(buffer)) != -1) {
                os.write(buffer, 0, bytesRead);
                os.flush();
                Thread.sleep(Config.artificalSendingDelayMs); // Simulate artificial delay if needed
            }

            readThread.join();

            if (!stopSending[0]) {
                System.out.println("\nFile " + file.getName() +" uploaded successfully.");
            } else {
                System.out.println("\nFile upload was stopped due to server error.");
            }

        } catch (IOException | InterruptedException e) {
            System.err.println("Unexpected Error: " + e.getMessage());
        }
    }

}
