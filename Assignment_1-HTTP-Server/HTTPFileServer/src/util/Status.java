package util;

public class Status {
    public int statusCode;
    public String message;

    public Status(int statusCode, String message) {
        this.statusCode = statusCode;
        this.message = message;
    }

    @Override
    public String toString() {
        return statusCode + " " + message;
    }
}
