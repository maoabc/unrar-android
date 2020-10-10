package com.github.maoabc.unrar;

import java.io.IOException;
import java.nio.ByteBuffer;

public abstract class PasswordCallback implements UnrarCallback {
    @Override
    public final void processData(ByteBuffer buffer, int len) {
    }

    @Override
    public void close() throws IOException {

    }
}
