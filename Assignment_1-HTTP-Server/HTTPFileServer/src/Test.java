import java.io.FileInputStream;
import java.io.IOException;

public class Test {

    public static void readFileInChunks(String filePath) {
        byte[] buffer = new byte[1024]; // buffer of 1024 bytes
        try (FileInputStream fis = new FileInputStream(filePath)) {
            int bytesRead;
            while ((bytesRead = fis.read(buffer)) != -1) {
                System.out.println("Read " + bytesRead + " bytes.");
                String chunk = new String(buffer, 0, bytesRead);
                System.out.println(chunk);
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public static void main(String[] args) {
        String filePath = "root/file1.txt";
        readFileInChunks(filePath);
    }
}
