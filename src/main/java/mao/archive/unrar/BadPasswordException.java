package mao.archive.unrar;

/**
 * Created by mao on 17-6-4.
 */
public class BadPasswordException extends Exception{
    public BadPasswordException() {
    }

    public BadPasswordException(String message) {
        super(message);
    }
}
