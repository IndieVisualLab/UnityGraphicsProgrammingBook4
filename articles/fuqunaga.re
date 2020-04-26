= Limit sets of Kleinian groups

//image[fuqunaga/intro0][]
//image[fuqunaga/intro2][]

本章ではクライン群の極限集合をシェーダーで描き、出来上がったフラクタル図形をアニメーションさせる方法を紹介します。
フラクタルのアニメーションといえば拡大縮小することで自己相似形を眺める面白みがありますが、
こちらの手法ではさらに直線と円周がなめらかに遷移する特徴的な動きが見られます。

本章のサンプルは@<br>{}
@<href>{https://github.com/IndieVisualLab/UnityGraphicsProgramming4}@<br>{}
の「KleinianGroup」になります。


== 円反転

まず図形の反転について紹介します。
直線を境に鏡写しのように反転した図形は線対称、点を中心に反転した場合は点対称、というのはおなじみだと思います。
さらに円に関しての反転というものが存在します。
２次元平面を円の内部と外部を互いに入れ替える操作になります。

#@#笑わない数学者いれれたら入れてみる

//image[fuqunaga/circle_inverse0][円反転@<m>{P \rightarrow P'}][scale=0.5]

円の中心@<m>{O},半径@<m>{r}の円に関しての反転は、@<m>{\left| OP\right|}と@<m>{\left| OP'\right|}が同じ向きのまま次の式を満たすように@<m>{P}を@<m>{P'}へ移す操作になります。

//texequation{
\left| OP\right| \cdot \left| OP'\right| =r^{2}
//}

円周付近では内側と外側が歪んだ線対称のように入れ替わっているように見え、円周から遥かに離れた無限遠と円の中心が入れ替わる形になります。
面白いのは円の外側の直線を円反転した場合で、円に近い付近では円周を挟んだ内側に移りそこから離れていくと無限遠まで続く、つまり反転後は円の中心に繋がる形になります。


//image[fuqunaga/circle_inverse1][直線の円反転][scale=0.7]

これは円の内側に小さい円として現れます。直線も半径無限の円と捉えると、円反転は円の内側と外側の円同士を入れ替える操作と言えます。


=== 数式で表す

複素平面上の単位円の円反転を式で表すと次のようになります。

//texequation{
z\rightarrow\dfrac {1}{\overline {z}}
//}

@<m>{z}は複素数で@<m>{\overline {z\}}はその複素共益です。

次のように式変形してみると@<m>{z}の長さの２乗分の１で@<m>{z}をスケールしている操作であることがわかります。

//texequation{
z\rightarrow \dfrac {1}{\overline {z}}=\dfrac {1}{x-iy}=\dfrac {x+iy}{\left( x-iy\right) \left( x+iy\right) }=\dfrac {x+iy}{x^{2}+y^{2}}=\dfrac {z}{\left| z\right| ^{2}}
//}


複素平面での図形操作として、

 * 複素数の和：平行移動
 * 複素数の積：回転（と拡大縮小）

と認識されている方も多いかと思いますが、ここに新しく割り算を含む操作が入ってきた形になります。


== メビウス変換

複素平面における変換に割り算まで含めて一般化した形がメビウス変換@<fn>{mebius}です。
//footnote[mebius][メビウスの帯でおなじみに数学者アウグスト・フェルディナント・メビウスに由来します。]

//texequation{
z\rightarrow \dfrac {az+b}{cz+d}
//}

@<m>{a,b,c,d}はすべて複素数です。


== ショットキー群、クライン群

メビウス変換を繰り返し用いてフラクタル図形を作ることを考えます。

互いに交わらない四組の円@<m>{D_A},@<m>{D_a},@<m>{D_B},@<m>{D_b}を用意します。
まずこのうち二組の円に注目し@<m>{D_A}の外部を@<m>{D_a}内部に、@<m>{D_A}の内部を@<m>{D_a}外部に移すメビウス変換@<m>{a}を作ります。
同様に別の二組の円@<m>{D_B}、@<m>{D_b}からメビウス変換@<m>{b}を作ります。
またそれぞれの逆変換@<m>{A},@<m>{B}も用意します。

//image[fuqunaga/schottky][]

こうしてできた４つのメビウス変換@<m>{a},@<m>{A},@<m>{b},@<m>{B}をどのような順番であれ合成した変換全体（たとえば@<m>{aaBAbbaB}）を、「@<m>{a},@<m>{b}を元にした@<strong>{ショットキー群}@<fn>{Schottky}」と呼びます。
//footnote[Schottky][このような群を最初に考案した数学者フリードリッヒ・ヘルマン・ショットキーに由来します。]

これをさらに一般化しメビウス変換からなる離散群のことを@<strong>{クライン群}と呼びます。どうもこちらの呼び方のほうが広く使わている印象です。


== 極限集合

ショットキー群の像を表示していくと、円の内部に円がありその内部にも円があり、と無限に続く円が現れます。
これらの集合を「@<m>{a},@<m>{b}におけるショットキー群の@<strong>{極限集合}」と呼びます。
この@<strong>{極限集合}を描くことが本章の目的になっています。


== Jos Leys氏のアルゴリズム
 
=== 概要

シェーダーで極限集合を描画する方法について紹介していきます。
変換の組み合わせが無限に続いてしまうので愚直に実装すると大変なのですが、
Jos Lays氏がこのためのアルゴリズムを公開している@<fn>{josleys}のでこれに沿ってやってみます。


まず２つのメビウス変換を用意します。

//texequation{
a: z\rightarrow \dfrac {tz-i}{-iz}
//}
//texequation{
b: z\rightarrow z+2
//}

@<m>{t}は複素数@<m>{u+iv}です。この値をパラメータとして変化させることで図形の形状を変えることができます。


変換@<i>{a}を詳しく見ていくと、

//texequation{
a: z\rightarrow \dfrac {tz-i}{-iz}=\dfrac {t}{-i}+\dfrac {1}{z}=\dfrac{1}{z}+(-v+iu)
//}

//texequation{
\dfrac {1}{z}=\dfrac {1}{x+iy}=\dfrac {x-iy}{(x+iy)(x-iy)}=\dfrac {x-iy}{x^{2}+y^{2}}=\dfrac {x-iy}{\left|z\right|^{2}}
//}

したがって、

//texequation{
a: z\rightarrow \dfrac {tz-i}{-iz}=\dfrac{x-iy}{\left|z\right|^{2}}+(-v+iu)
//}

なので、

 1. 単位円における円反転し、
 1. ｙの符号を反転し、
 1. @<m>{-v+iu}平行移動する

という操作になっていることがわかります。

変換@<i>{a},@<i>{b}とその逆変換を用いた極限集合は大小の円がつながった次のような帯状の図形になります。

//image[fuqunaga/limit_set][極限集合]


この図形の特徴について詳しく見ていきます。

//image[fuqunaga/limit_set_detail][]

@<m>{0\leq y\leq u}の帯状になっており、左右方向にはLine1,2で区切られた平行四辺形が繰り返している形になっています。
Line1は点@<m>{(1,0)},点@<m>{(1-v,u)}を通る直線で、Line2は点@<m>{(-1,0)},点@<m>{(-1-v,u)}を通る直線です。
Line3は図形を上限に分ける線になり平行四辺形の中においてこの線で分けられた上下の図形は点@<m>{z= -\dfrac{v\}{2\}+\dfrac{iu\}{2\}} において点対称になっています。


=== アルゴリズム

任意の点が極限集合に含まれるかどうかを判定します。 
左右には平行四辺形領域が繰り返している、上下にはLine3を境界に点対称になっていることを利用し
各点における判定を最終的には中心下半分の図形の判定に持っていきます。

ある点について次のように処理していきます。

 * もしLine1よりも右にあるなら平行四辺形範囲に収まるように左に移動
 * もしLine2よりも左にあるなら同様に右に移動
 * Line3よりも上にあるなら点対称であることを利用しLine3の下になるように対象点に移動
 * Line3よりも下にある点には変換@<i>{a}を適用する

直線@<m>{y=０}に接している一番大きい円は、直線@<m>{y=u}を単位円で反転したものになります。
この中の点に変換@<i>{a}をかけると@<m>{y<0}となり@<m>{0\leq y\leq u}の帯から外れます。
したがって、

//quote{
ある点に変換@<i>{a}をかけたら@<m>{y<0}となった = ある点はこの一番大きな円に含まれる = 極限集合に含まれる
//}

として判定を行います。

逆に含まれない場合はどうでしょうか。
上記手順を繰り返しても@<m>{0\leq y\leq u}の帯から出られず、最終的にはLine3を挟んだ２点の移動を繰り返す形になります。
したがって２つ前と同じ点だった場合は極限集合には含まれない点であると判定できます。

まとめると次のように処理していく形になります。

 1. @<m>{y<0},@<m>{u<y}であれば範囲外
 1. Line1より右、Line2より左にあれば中心の平行四辺形へ移動
 1. Line3より上にあれば中心点について反転
 1. 変換@<i>{a}を適用
 1. もし@<m>{y<0}なら極限集合と判定
 1. ２つ前の点と同じなら極限集合ではないと判定
 1. どちらでもなければ2.に戻る

== 実装

それではコードを見ていきます。

//emlist[KleinianGroup.cs]{
private void OnRenderImage(RenderTexture source, RenderTexture destination)
{
    material.SetColor("_HitColor", hitColor);
    material.SetColor("_BackColor", backColor);
    material.SetInt("_Iteration", iteration);
    material.SetFloat("_Scale", scale);
    material.SetVector("_Offset", offset);
    material.SetVector("_Circle", circle);

    Vector2 uv = kleinUV;
    if ( useUVAnimation)
    {
        uv = new Vector2(
            animKleinU.Evaluate(time),
            animKleinV.Evaluate(time)
            );           
    }
    material.SetVector("_KleinUV", uv);
    Graphics.Blit(source, destination, material, pass);
}
//}

C#側はKleinianGroup.csでインスペクタで設定したパラメータを渡しつつマテリアルを描画する@<code>{OnRenderImage()}のみの処理になっています。

シェーダーを見てきましょう。

//emlist[KleinianGroup.shader]{
#pragma vertex vert_img
//}

VertexシェーダーはUnity標準のvert_imgを使っています。
メインはFragmentシェーダーです。
Fragmentシェーダーは３つ用意してありそれぞれ別のパスになっています。
１つ目が標準的なもの、２つ目がぼかし処理が入り少し見た目がきれいになったもの、３つ目が後述のさらに円反転を加えたものになっています。
KleinianGroup.csでどのパスを使うか選択できるようになっています。ここでは１つ目のものを見ていきます。


//emlist[KleinianGroup.shader]{
fixed4 frag (v2f_img i) : SV_Target
{
    float2 pos = i.uv;
    float aspect = _ScreenParams.x / _ScreenParams.y;
    pos.x *= aspect;
    pos += _Offset;
    pos *= _Scale;

    bool hit = josKleinian(pos, _KleinUV);
    return hit ? _HitColor : _BackColor;
}
//}

@<code>{_ScreenParams}からアスペクト比を求めてpos.xに乗算しています。
これでposで表される画面上の領域は0≦y≦1で、xはアスペクト比に応じた範囲になります。
さらにC#側から渡される@<code>{_Offset},@<code>{_Scale}を適用することで表示する位置と範囲を調整できるようにしています。
@<code>{josKleinian()}で極限集合の可否判定を行い出力する色を決定しています。

@<code>{josKleinian()}を詳しく見ていきます。

//emlist[KleinianGroup.shader]{
bool josKleinian(float2 z, float2 t)
{
    float u = t.x;
    float v = t.y;

    float2 lz=z+(1).xx;
    float2 llz=z+(-1).xx;

    for (uint i = 0; i < _Iteration ; i++) 
    {
        ～
//}

点zとメビウス変換のパラメータtを受け取って、ｚが極限集合に含まれるかどうか判定する関数です。
lz,llzは、集合の外部であることを示す「２つ前と同じ点」判定用の変数です。
とりあえず開始時のzと異なりまたお互いも異なるように値を初期化しています。
@<code>{_Iteration}は手順を繰り返す最大回数です。細部を拡大して見ていくのでなければそれほど大きな値でなくても十分だと思います。


//emlist[KleinianGroup.shader]{
// wrap if outside of Line1,2
float offset_x = abs(v) / u * z.y;
z.x += offset_x;
z.x = wrap(z.x, 2, -1);
z.x -= offset_x;
//}

//emlist[KleinianGroup.shader]{
float wrap(float x, float width, float left_side){
    x -= left_side; 
    return (x - width * floor(x/width)) + left_side;
}
//}

ここが
//quote{
Line1より右、Line2より左にあれば中心の平行四辺形へ移動
//}
 の部分になります。

@<code>{wrap()}は点の位置、長方形の横幅、長方形の左端の座標を受け取り、左右にはみ出している点を収める関数です。
 offset_xで平行四辺形を長方形に変換し@<code>{wrap()}で範囲内に収め、再度offset_xで平行四辺形に戻す処理になっています。


次にLine3の判定です。

//emlist[KleinianGroup.shader]{
//if above Line3, inverse at (-v/2, u/2)
float separate_line = u * 0.5 
    + sign(v) *(2 * u - 1.95) / 4 * sign(z.x + v * 0.5)
    * (1 - exp(-(7.2 - (1.95 - u) * 15)* abs(z.x + v * 0.5)));

if  (z.y >= separate_line)
{
    z = float2(-v, u) - z;
}
//}

@<code>{separate_line}を求めている部分がLine3の条件式になります。この部分は導出がよくわからず図形の対称性からおおよそで求めているものかなと思います。
複雑な図形になるように突き詰めたtの値によっては上下の図形がギザギザに噛み合うことがありちゃんと分割するにはこの条件式では不十分なこともありますが、
おおよそ一般的な形では有効なので今回はこのまま使用します。


//emlist[KleinianGroup.shader]{
z = TransA(z, t);
//}

//emlist[KleinianGroup.shader]{
float2 TransA(float2 z, float2 t){
    return float2(z.x, -z.y) / dot(z,z) + float2(-t.y, t.x);
}
//}

いよいよ点zにメビウス変換@<i>{a}を適用します。
コーディングしやすいように前述の式変形を利用して、

//texequation{
a: z\rightarrow \dfrac {tz-i}{-iz}=\dfrac{x-iy}{\left|z\right|^{2}}+(-v+iu)
//}

これを実装しています。


//emlist[KleinianGroup.shader]{
//hit!
if (z.y<0) { return true; }
//}

変換の結果、@<m>{y<0}なら極限集合判定、

//emlist[KleinianGroup.shader]{
//2cycle
if(length(z-llz) < 1e-6) {break;}

llz=lz;
lz=z;
//}

２つ前の点とほぼ同値なら極限集合ではないと判定します。
また、@<code>{_Iteration}繰り返しても判定結果が出ない場合も極限集合ではないと判定しています。


以上でシェーダーの実装は完了です。
パラメータtは@<m>{(2,0)}が最も標準的な値で@<m>{(1.94,0.02)}付近で面白い形になりやすいです。
サンプルプロジェクトではKleinianGroupDemo.csのインスペクタ上で編集できるのでぜひ遊んでみてください。


== さらに円反転

極限集合の表示は以上なのですがアニメーションとして面白くするために最後に強力なトッピングを加えます。
@<code>{josKleinian()}にわたす前にポジションを円反転します。
円反転は、円の外側の無限に広がる領域と内側を入れかえ、しかも円は円として移されるのでした。
そして極限集合は無数の円で構成されています。
この反転円を動かしたり半径を変えたりすることで、フラクタルの面白さを活かしつつ予想もできないような不思議な見た目が出来上がります。

//emlist[KleinianGroup.shader]{
float4 calc_color(float2 pos)
{
    bool hit = josKleinian(pos, _KleinUV);
    return hit ? _HitColor : _BackColor;
}

～
float4 _Circle;

float2 circleInverse(float2 pos, float2 center, float radius)
{
    float2 p = pos - center;
    p = (p * radius) / dot(p,p);
    p += center;
    return p;
}

fixed4 frag_circle_inverse(v2f_img i) : SV_Target
{
    float2 pos = i.uv;
    float aspect = _ScreenParams.x / _ScreenParams.y;
    pos.x *= aspect;
    pos *= _Scale;
    pos += _Offset;

    int sample_num = 10;
    float4 sum;
    for (int i = 0; i < sample_num; ++i)
    {
        float2 offset = rand2n(pos, i) * (1/_ScreenParams.y) * 3;
        float2 p = circleInverse(pos + offset, _Circle.xy, _Circle.w);
        sum += calc_color(p);
    }

    return sum / sample_num;
}
//}

これは３つ目のパスで定義されている円反転を加えたFragmentシェーダーです。
@<code>{sample_num}のループは周辺のピクセルも計算して少しぼかすことで見た目を綺麗にする処理になります。
@<code>{calc_color()}がいままでの色を計算する処理になりその前に@<code>{ciecleInverse()}で円反転しています。

KleinianGroupCircleInverseシーンではこちらのシェーダーのパラメータをAnimatorで変化させることでフラクタルらしいアニメーションが動作するようになっています。


== まとめ

本章ではクライン群の極限集合をシェーダーで描く方法とさらに円反転の用いることでより面白いフラクタル図形の作り方を紹介しました。
フラクタルやメビウス変換は普段馴染みのない分野でとっつきにくい部分もあったのですが次々と予想もしないような模様が動いてく様はとても刺激的でした。
よかったらみなさんもチャレンジしてみてください！


== 参考

 * インドラの真珠@<fn>{indra} @<br>{}
 最強のバイブルです。ちょっと値が張り、内容も高度なので以下のところを見てより深く理解したくなったら手に取るのがおすすめです。

 * Mathe Vital @<fn>{mathevital} @<br>{}
 インドラの真珠のエッセンスを抜き出してわかりやすく紹介されています。まずここから見ていくと良いかと思います。

 * 今回紹介したJos Leys氏のアルゴリズム@<fn>{josleys}

 * Jos Leys氏のShadertoy @<fn>{jos_shadertoy} @<br>{}
 貴重なシェーダー実装の例です

 * Morph @<fn>{morph} @<br>{}
 @soma_arcさん@<fn>{soma}がTokyoDemoFest2018@<fn>{tdf}で発表されていた作品です。今回私自身クライン群を学ぶきっかけになりました。こちらも貴重なシェーダー実装例。

//footnote[indra][https://www.amazon.co.jp/dp/4535783616]
//footnote[mathevital][http://userweb.pep.ne.jp/hannyalab/MatheVital/IndrasPearls/IndrasPearlsindex.html]
//footnote[josleys][http://www.josleys.com/articles/Kleinian%20escape-time_3.pdf]
//footnote[jos_shadertoy][https://www.shadertoy.com/user/JosLeys]
//footnote[morph][https://www.shadertoy.com/view/MlGfDG]
//footnote[soma][https://twitter.com/soma_arc]
//footnote[tdf][http://tokyodemofest.jp/2018/]