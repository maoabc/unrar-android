package mao.archive.unrar;

import java.io.IOException;

/**
 * Created by mao on 16-10-10.
 */

public class RarException extends IOException {
    public RarException() {
    }

    public RarException(String message) {
        super(message);
    }
}
