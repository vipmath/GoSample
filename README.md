# モンテカルロ木探索(UCTアルゴリズム)による囲碁プログラム

C++言語で作成したモンテカルロ木探索(UCTアルゴリズム)を使用した囲碁プログラムです。

Windows用のGUIプログラムで、Visual Studio Community 2015でビルドできます。

GTPプロトコルに対応しています。

## 実行方法

GoSample.exeを実行して、Startボタンをクリックすると、
対戦が始まります。

GTP対応クライアントから実行する場合は、
引数に「-gtp」を指定します。

> GoSample.exe -gtp

プレイアウト回数を変更するには、引数に数字を指定します。
デフォルトは、1000回です。

> GoSample.exe 2000
