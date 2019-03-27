package mao.archive.unrar;

import java.io.IOException;

import androidx.annotation.Keep;

/**
 * Created by mao on 16-10-10.
 */

@Keep
public class RarException extends IOException {
    public RarException() {
    }

    public RarException(String message) {
        super(message);
    }
}
