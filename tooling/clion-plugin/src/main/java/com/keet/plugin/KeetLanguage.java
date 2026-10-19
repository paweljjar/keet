package com.keet.plugin;

import com.intellij.lang.Language;

public class KeetLanguage extends Language {
    public static final KeetLanguage INSTANCE = new KeetLanguage();

    private KeetLanguage() {
        super("Keet");
    }

    @Override
    public String getDisplayName() {
        return "KeetScript";
    }

    @Override
    public boolean isCaseSensitive() {
        return true;
    }
}
