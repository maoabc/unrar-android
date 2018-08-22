package mao.archive.unrar;

/**
 * 批量解压过滤
 * Created by mao on 17-6-7.
 */
public interface ExtractFilter {
    boolean accepts(RarEntry entry);
}
