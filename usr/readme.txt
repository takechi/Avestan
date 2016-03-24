--------------------------------------------------------------------------------
form.xml		
profile.ini		雑多なオプションの指定
main.ico		Avestaのアプリケーションアイコンとして使われます。
keyboard-global.xml	「Windows全体」でのキーボード入力
keyboard-form.xml	「Avesta全体」でのキーボード入力
keyboard-folder.xml	「フォルダ」でのキーボード入力
keyboard-tree.xml	「ツリー」でのキーボード入力
keyboard-preview.xml	「プレビュー」でのキーボード入力
mouse-folder.xml	マウスでの入力
menu.xml/png		メニューの構成とアイコン
tool.xml/png		ツールバーの構成とアイコン
status.xml/png		ステータスバーの構成とアイコン
sysmenu.xml/png		システムメニューの構成とアイコン
link.xml		リンクバーの構成。

--------------------------------------------------------------------------------
【無効／ホット処理の状態のアイコンについて】

*.png のアイコンイメージは、
*-disable.png / *-hot.png を作成すると、
それぞれ「無効化」「ホット」イメージとして使用されます。

version 0.2.4.2 現在、tool.png / status.png のみ対応しています。
例えば tool の場合、tool.png / tool-disable.png / tool-hot.png で指定します。
見つからない場合、
  - 無効化状態 : 通常アイコンをグレースケールにしたもの
  - ホット状態 : 通常アイコンと同じ
になります。

--------------------------------------------------------------------------------
【処理の委譲について】
マウスやキーボードの入力は、まず、現在フォーカスされているウィンドウで処理されます。
処理が行われなかった場合は、階層の上のウィンドウへ順に委譲されます。
現在の構成は、以下のようになっています。

- frame    | フレームウィンドウ
  - folder | フォルダウィンドウ
  - tree   | ツリーバー

また、"global" のみ特別扱いで、他の全てよりも優先されます。
"global" は、キーボードのみ指定可能で、Avestaがアクティブでなくても使用できます。

--------------------------------------------------------------------------------
【キーマップで認識できるキー名】
大文字・小文字は区別しません。
日本語キーボードで動作確認。英語キーボードだと記号系がずれるかも。

修飾キー (bind@modifier)
  A ⇒ Alt
  C ⇒ Control
  S ⇒ Shift
  W ⇒ Windows

キー名  │エイリアス (bind@key)
────┼──────────────────
0-9, A-Z│0-9, a-z
F1-24   │F1-24
Num 0-9 │num 0-9
Num .   │num ., decimal
Num +   │num +, add
Num -   │num -, sub, subtract
Num *   │num *, mul, multiply
Num /   │num /, div, divide
,       │,
-       │-
.       │.
/       │/
:       │:
;       │;
@       │@
[       │[
\       │\
]       │]
^       │^
_       │_
|       │sep, separator
Enter   │enter, return
Esc     │esc, escape
BS      │bs, back, backspace
Home    │home
End     │end
Ins     │ins, insert
Del     │delete
Tab     │tab
Space   │space
PageDown│pagedown, next
PageUp  │pageup, prior
←      │←, left
↑      │↑, up
→      │→, right
↓      │↓, down
カナ    │カナ, カタカナ, ローマ字
半角    │半角, 漢字
変換    │変換
無変換  │無変換

--------------------------------------------------------------------------------

【メニュー用ビットマップの並び方】
'*' は、今のところアプリ側に相当するコマンドがないもの

ID│説明                                │想定コマンド
─┼──────────────────┼───────────────────
 0│                                    │
 1│取り消し                            │Current.Undo
 2│最近閉じたフォルダ                  │File.MRU
 3│切り取り                            │Current.Cut
 4│コピー                              │Current.Copy
 5│貼り付け                            │Current.Paste
 6│削除                                │Current.Delete
 7│戻る                                │Current.Go.Back
 8│進む                                │Current.Go.Forward
 9│上へ                                │Current.Go.Up
10│場所を指定して移動                  │Current.MoveTo
11│場所を指定してコピー                │Current.CopyTo
12│フォルダ（ツリーの表示切替）        │Side.Show(toggle)
13│開く                                │File.Open
14│保存                                │File.Save
15│新規作成                            │Current.New
16│全てのファイルを表示                │Current.ShowAllFiles
17│更新                                │Current.Refresh
18│*中止                               │
19│ホーム                              │Current.Go.Home
20│検索                                │Current.Find
21│*お気に入り                         │
22│プレビュー                          │Preview.Show(toggle)
23│*メール（メールで転送？）           │
24│*ウィンドウ（汎用）                 │
25│ウィンドウを閉じる                  │Current.Close
26│表示形式の変更                      │
27│新しいフォルダを作成                │Current.NewFolder
28│ウィンドウを隠す                    │Current.Show(false)
29│ウィンドウをロック                  │Current.Lock(toggle)
30│ゴミ箱を空にする                    │RecycleBin.Clear
31│                                    │
32│縮小版                              │Current.Mode.Thumbnail
33│並べて表示                          │Current.Mode.Tile
34│アイコン                            │Current.Mode.Icon
35│一覧                                │Current.Mode.List
36│詳細                                │Current.Mode.Details
37│横に並べる                          │Arrange.Horz
38│縦に並べる                          │Arrange.Vert
39│自動的に並べる                      │Arrange.Auto
40│全て閉じる                          │All.Close
41│右を閉じる                          │Rights.Close
42│左を閉じる                          │Lefts.Close
43│奥を閉じる                          │Hidden.Close
44│前を閉じる                          │Shown.Close
45│重複を閉じる                        │Duplicate.Close
46│他を閉じる                          │Others.Close
47│                                    │
48│フォルダをツリーに同期させる        │Tree.Sync
49│フォルダをツリーに反映させる        │Tree.Reflect
50│ツリーを完全同期（双方向）          │Tree.AutoSync;Tree.AutoReflect 
51│エクスプローラの取り込み            │Explorer.Import 
52│エクスプローラとして切り離す        │Current.Export 
53│すべてを表示                        │All.Show(true)
54│チェックボックス                    │Option.CheckBox
55│常に手前に表示                      │Option.AlwaysTop
─┴──────────────────┴───────────────────
