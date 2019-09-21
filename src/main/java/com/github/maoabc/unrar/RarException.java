package com.github.maoabc.unrar;

import androidx.annotation.Keep;

import java.io.IOException;

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
