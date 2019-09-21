package com.github.maoabc.unrar;

import java.io.IOException;

public abstract class PasswordCallback implements UnrarCallback {
    @Override
    public final void processData(byte[] b, int off, int len) {
    }

    @Override
    public void close() throws IOException {

    }
}
