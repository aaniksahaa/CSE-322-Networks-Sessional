package util;
import java.io.FileWriter;
import java.io.IOException;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;

public class LogWriter {
    private FileWriter logWriter;

    public LogWriter(String fileName) throws IOException {
        logWriter = new FileWriter(fileName, true);
    }

    public void writeLog(String logText) {
        try {
            logWriter.write(logText);
            logWriter.flush();
        } catch (IOException e) {
            System.out.println("Error writing to log file: " + e.getMessage());
        }
    }

    public void close() {
        try {
            if (logWriter != null) {
                logWriter.close();
            }
        } catch (IOException e) {
            System.out.println("Error closing log file: " + e.getMessage());
        }
    }

    public static void main(String[] args) {
        try {
            LogWriter logger = new LogWriter("log.txt");
            logger.writeLog("Demo");
            logger.close();
        } catch (IOException e) {
            System.out.println("Error creating log writer: " + e.getMessage());
        }
    }
}

