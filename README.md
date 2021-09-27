## 概要
Linux環境でフロッピーディスクをD88形式にダンプおよびリストアを行うためのツールです。

## 開発環境
Ubuntu Desktop 20.04 LTS

## ビルド
     $ make

make installは未実装です。

/dev/fd0のアクセス許可が必要です。一般ユーザーで動作させる場合は、該当ユーザーをdiskグループに所属させるなどしてアクセス許可を与えてください。

## 使用方法
fdm [dump|restore] filename options

    -h              # 使用方法の表示
    -v              # 詳細モード
    -w[on|off]      # ライトプロテクトフラグの設定
    -m<type>        # メディアタイプ(2D/2DD/2HD/1D/1DD) デフォルト:2HD
    -C<start>-<end> # シリンダーの範囲
    -S<side>        # サイドセレクト(0/1/2) デフォルト:2(両面)
    -M<multiplier>  # シーク倍率(2HDドライブで2Dを読む場合は2を指定)
    -D<rpm>,<kbps>  # ドライブの回転数、転送レートを指定(GAP3・アンフォーマットパラメータに使用)
    -R<drate>       # DRATEレジスタの設定値

## 実行例
     $ ./fdm dump test.d88
     $ ./fdm restore test.d88