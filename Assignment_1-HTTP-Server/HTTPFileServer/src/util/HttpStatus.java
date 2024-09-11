package util;

public class HttpStatus {
    public int statusCode;
    public String message;

    public HttpStatus(int statusCode, String message) {
        this.statusCode = statusCode;
        this.message = message;
    }

    @Override
    public String toString() {
        return statusCode + " " + message;
    }
}
