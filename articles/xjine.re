= StarGlow

//image[xjine/StarGlow01][輝度の高い部分から伸びる光線]

強い光の照り返しが生じるときに伸びる光線、LightLeak だったり LightStreak だったり、StarGlow と呼ばれますが、
ポストエフェクトでこれを表現してみましょう。ここでは便宜上 StarGlow(スターグロー) と呼びます。

ここで紹介するこのポストエフェクトは GDC 2003 で Masaki Kawase 氏によって発表されたものです。

本章のサンプルは@<br>{}
@<href>{https://github.com/IndieVisualLab/UnityGraphicsProgramming4}@<br>{}
の「StarGlow」です。

== STEP 1 : 輝度画像を生成する

//image[xjine/StarGlow02][オリジナルのイメージ]
//image[xjine/StarGlow03][高い輝度の画素だけを検出したイメージ]

まずは明るい部分だけを検出した画像(輝度画像)を作りましょう。一般的なグローについても同じような処理を必要としますね。
輝度画像を作るためのシェーダとスクリプトのソースコードは次のようになります。シェーダパスが 1 である点に注意してください。

//emlist[StarGlow.cs]{
RenderTexture brightnessTex
= RenderTexture.GetTemporary(source.width  / this.divide,
                             source.height / this.divide,
                             source.depth,
                             source.format);
…
base.material.SetVector
(this.idParameter, new Vector3(threshold, intensity, attenuation));

Graphics.Blit(source, brightnessTex, base.material, 1);
//}

//emlist[StarGlow.shader]{
#define BRIGHTNESS_THRESHOLD _Parameter.x
#define INTENSITY            _Parameter.y
#define ATTENUATION          _Parameter.z
…
fixed4 frag(v2f_img input) : SV_Target
{
    float4 color = tex2D(_MainTex, input.uv);
    return max(color - BRIGHTNESS_THRESHOLD, 0) * INTENSITY;
}
//}

輝度の算出方法についてはさまざまな方法がありますが、古典的なグローの実装などでも使われている算出方式をそのまま活用しました。
他に一度グレースケールにしてから輝度を比較するなどの処理を行っているシェーダも見かけます。

@<code>{BRIGHTNESS_THRESHOLD} は、輝度と判定する閾値、@<code>{INTENSITY} は輝度に乗算するバイアスです。
@<code>{color} に与えられる値が大きいほど、つまり明るい色ほど、大きな値が返りやすいことを確認してください。
閾値が大きいほど、0 より大きな値が返る確率は減りますね。またバイアスが大きいほど、より強い輝度画像が得られるようになっています。

@<code>{ATTENUATION} についてはこの時点では使いません。
一度にパラメータとして渡した方が CPU → GPU 間での値のやり取りにかかるオーバーヘッドが小さくなるために、
ここでは @<code>{Vector3} としてまとめて渡しています。

この時点でもっとも重要なのは、輝度画像をサイズの小さい RenderTexture として取得している点です。

一般に、ポストエフェクトは解像度が大きくなればなるほど、Fragment シェーダの呼び出し数、その算出回数が増えて負荷が大きくなります。
さらにグロー効果については繰り返し処理が発生するために処理負荷はさらに大きくなるのです。
スターグローもこの例に漏れません。したがって、効果の解像度を必要な分まで下げることによって、かかる負荷を軽減します。

繰り返し処理については後述します。

== STEP 2 : 輝度画像に指向性ブラーをかける

//image[xjine/StarGlow04][斜めに引き伸ばされた輝度画像]

STEP1 で得られた輝度画像にブラーをかけて引き伸ばします。
この引き伸ばし方を工夫することによって、一般的なグローとは異なる鋭く伸びる光線を表現します。

一般的なグローの場合は全方向に向かってガウス関数による引き伸ばしをしますが、
スターグローの場合は指向性のある引き伸ばしをする、ということです。

//emlist[StarGlow.cs]{
Vector2 offset = new Vector2(-1, -1);
// (Quaternion.AngleAxis(angle * x + this.angleOfStreak,
//                       Vector3.forward) * Vector2.down).normalized;

base.material.SetVector(this.idOffset, offset);
base.material.SetInt   (this.idIteration, 1);

Graphics.Blit(brightnessTex, blurredTex1, base.material, 2);

for (int i = 2; i <= this.iteration; i++)
{
    繰り返し描画
}
//}

実際の処理とは異なりますが、ここでは説明のために @<code>{offset = (1, 1)} としましょう。
さらに、@<code>{offset} と @<code>{iteration} をシェーダに渡していることに注目してください。

続いてスクリプト側ではシェーダパス 2 で繰り返し描画を実行していますが、簡単に考えるために、ここで一度シェーダの方に移りましょう。
シェーダパス 2 で描画していることに注意してください。

//emlist[StarGlow.shader]{
int    _Iteration;
float2 _Offset;

struct v2f_starglow
{
    …
    half   power  : TEXCOORD1;
    half2  offset : TEXCOORD2;
};

v2f_starglow vert(appdata_img v)
{
    v2f_starglow o;
    …
    o.power  = pow(4, _Iteration - 1);
    o.offset = _MainTex_TexelSize.xy * _Offset * o.power;
    return o;
}

float4 frag(v2f_starglow input) : SV_Target
{
    half4 color = half4(0, 0, 0, 0);
    half2 uv    = input.uv;

    for (int j = 0; j < 4; j++)
    {
        color += saturate(tex2D(_MainTex, uv)
               * pow(ATTENUATION, input.power * j));
        uv += input.offset;
    }

    return color;
}
//}

まずは Vertex シェーダから確認します。@<code>{power} は引き伸ばされるときに輝度が減衰する力、
@<code>{offset} はブラーによって輝度を引き伸ばす方向を示します。後述する Fragment シェーダで参照します。

Vertex シェーダ内でこれらを算出しているのは、Fragment シェーダ内において共通の値を参照するためです。
Fragment シェーダ内で逐次算出するのは演算回数が増えるので良くありませんね。

ここで @<code>{_Iteration = 1} です。したがって @<code>{power = 4^0 = 1} になります。
そうると @<code>{offset = 画素の大きさ * (1, 1)} が得られます。

これで、ちょうど 1 画素分だけオフセット方向にズレた画素をサンプリングするための準備が整いました。

次に Fragment シェーダです。参照する @<code>{uv} を @<code>{offset} の分だけ 1 つずつ移動しながら 4 回参照し、
その画素の値を合算しています。ただし画素の値には @<code>{pow(ATTENUATION, input.power * j)} が乗算されていますね。

@<code>{ATTENUATION} はその画素の値をどれくらい減衰するかを表す値です。引き伸ばしたときのボケ、減衰具合に影響します。

仮に @<code>{ATTENUATION = 0.7} とすれば、最初にサンプリングする画素は * 0.7,
次にサンプリングする画素は 0.7 ^ 2 = * 0.49 となりますね。図で見るとイメージが付きやすいです。

//image[xjine/StarGlowBlur][ブラーがかかる過程を表した図]

左の図が減衰前のオリジナルの輝度画像です。@<code>{_MainTex} に相当します。
今 Fragment シェーダに与えられる @<code>{uv} が参照する画素を左下の START としましょう。
@<code>{offset = (1, 1)} ですから、4 回のイテレーションで参照する画素は右上の END までです。

画素の中の値は、その画素のもつ輝度の値です。START から 3 つが 0 で、END だけが 1 です。
先のソースコードはイテレーションするたびに減衰率が上がりますから、ちょうど真ん中の図のようなイメージになります。
これを合算すると、START の画素が最終的に得られる値は @<code>{color = 0.34} になります。

同じようにして Fragment シェーダが各画素を処理していけば、右の図のような結果が得られることが分かると思います。
ブラーのようなグラデーションが得られていますね。また @<code>{offset} が引き伸ばす方向を示すパラメータであると先に説明していますが、
見た目上の効果としては、指定した値と反対方向に伸びることになります。

=== 繰り返してさらに引き伸ばす

//image[xjine/StarGlow05][さらに引き伸ばされた輝度画像]

少しだけスクリプト側に話を戻しましょう。先までの説明は @<code>{this.iteration} ないし @<code>{_Iteration} が 1 であるとしていました。
実際には RenderTexture を入れ替えながら、任意の回数だけ同じ処理を繰り返しています。

//emlist[StarGlow.cs]{
Vector2 offset = new Vector2(-1, -1);

base.material.SetVector(this.idOffset, offset);
base.material.SetInt   (this.idIteration, 1);

Graphics.Blit(brightnessTex, blurredTex1, base.material, 2);

== ここから上が先までの解説に相当する == 

for (int i = 2; i <= this.iteration; i++)
{
    base.material.SetInt(this.idIteration, i);

    Graphics.Blit(blurredTex1, blurredTex2, base.material, 2);

    RenderTexture temp = blurredTex1;
    blurredTex1 = blurredTex2;
    blurredTex2 = temp;
}
//}

同じパスを使って同じ処理を繰り返していますから、得られる効果は変わりません。
ただし、シェーダパラメータの @<code>{_Iteration} の値が大きくなりますね、そうすると、先に説明したシェーダ内の減衰率が上がります。
また、入力される画像はすでに引き伸ばされているブラー画像になります。

端的にいえば、この繰り返し処理によって、1 回目よりもさらに伸びたブラー画像が @<code>{blurredTex1} に得られます。

この処理はコストがかかるので、現実的には繰り返し回数は精々 3 回くらいとは思います。
また、シェーダ内のイテレーションは 4 回ですが、この値は Kawase 氏の発表で提案されているものです。

== STEP 2.5 : 複数方向に伸びるブラー画像を合成する

//image[xjine/StarGlow06][別の方向に引き伸ばされた輝度画像]

ソースコード上では STEP2.5 はコメントしていないのですが、説明のために 2.5 としました。
先の説明では @<code>{offset = (1, 1)} としていましたが、複数方向に伸びる光線を作るために、
@<code>{offset} を回転してもう一度ブラーをかけましょう。

仮に @<code>{offset = (1, 1)} と反対方向に伸びる光線を定義するとすれば、@<code>{offset = (-1, -1)} ですね。
実際のソースコードでは光線の数だけ @<code>{offset} を回転していますが、説明の上では @<code>{offset = (-1, -1)} とします。

//emlist[StarGlow.cs]{
for (int x = 1; x <= this.numOfStreak; x++)
{
    Vector2 offset = Quaternion.AngleAxis(angle * x + this.angleOfStreak,
                                          Vector3.forward) * Vector2.down;
    offset = offset.normalized;

    for (int i = 2; i <= this.iteration; i++) {
        bluuredTex1 が繰り返し処理で伸ばされる
    }

    Graphics.Blit(blurredTex1, compositeTex, base.material, 3);
}
//}

最終的に得られたブラー画像 @<code>{blurredTex1} を、合成用の画像 @<code>{compositeTex} に出力しています。
@<code>{compositeTex} は複数方向に伸びるブラー画像がすべて合成された画像になりますね。

このとき、ブラー画像を合成するために使うシェーダーパスは 3 です。

//emlist[StarGlow.shader]{
Blend OneMinusDstColor One
…
fixed4 frag(v2f_img input) : SV_Target
{
    return tex2D(_MainTex, input.uv);
}
//}

このパスでは特別な処理は一切していませんが、@<code>{Blend} 構文を使って画像を合成しています。
合成方法は演出によって作り替えてしまってよいと思いますが、ここでは @<code>{OneMinusDstColor One} としました。ソフトな合成方法です。

== STEP 3 : ブラー画像を元画像に合成する

//image[xjine/StarGlow07][最終的に得られたブラー画像]

複数方向に延びるブラー画像が得られたら、あとは一般的なグローと同じように元画像にブラー画像を合成して出力します。
先の STEP 2.5 と同じように @<code>{Blend} 構文を使って合成して出力する方法でもよいですが、
ここでは @<code>{Blit} 回数の軽減と、合成方法の柔軟性のために合成用の @<code>{Pass 4} を使って合成するようにしています。

//emlist[StarGlow.cs]{
base.material.EnableKeyword(StarGlow.CompositeTypes[this.compositeType]);
base.material.SetColor(this.idCompositeColor, this.color);
base.material.SetTexture(this.idCompositeTex, compositeTex);

Graphics.Blit(source, destination, base.material, 4);
//}

//emlist[StarGlow.shader]{
#pragma multi_compile _COMPOSITE_TYPE_ADDITIVE _COMPOSITE_TYPE_SCREEN …
…
fixed4 frag(v2f_img input) : SV_Target
{
    float4 mainColor      = tex2D(_MainTex,      input.uv);
    float4 compositeColor = tex2D(_CompositeTex, input.uv);

    #if defined(_COMPOSITE_TYPE_COLORED_ADDITIVE)…
     || defined(_COMPOSITE_TYPE_COLORED_SCREEN)

    compositeColor.rgb
        = (compositeColor.r + compositeColor.g + compositeColor.b)
        * 0.3333 * _CompositeColor;

    #endif

    #if defined(_COMPOSITE_TYPE_SCREEN)…
     || defined(_COMPOSITE_TYPE_COLORED_SCREEN)

    return saturate(mainColor + compositeColor
                  - saturate(mainColor * compositeColor));

    #elif defined(_COMPOSITE_TYPE_ADDITIVE)…
       || defined(_COMPOSITE_TYPE_COLORED_ADDITIVE)

    return saturate(mainColor + compositeColor);

    #else

    return compositeColor;

    #endif
}
//}

@<code>{Blend} 構文は使っていないものの、スクリーン合成、加算合成をそのまま再現しています。
さらにここでは任意に乗算する色を追加することによって、色の強く付いたスターグローを表現できるようにしています。

== STEP 4 : リソースの開放

使ったリソースはすべて開放しましょう。特別解説することもありませんが、ソースコード上のサンプルにも記載してるので念のため。
実装環境などが限定的な場合は確保済みのリソースを使い回すなどの対応も可能でしょうが、ここではシンプルに Release します。

//emlist[StarGlow.cs]{
base.material.DisableKeyword(StarGlow.CompositeTypes[this.compositeType]);

RenderTexture.ReleaseTemporary(brightnessTex);
RenderTexture.ReleaseTemporary(blurredTex1);
RenderTexture.ReleaseTemporary(blurredTex2);
RenderTexture.ReleaseTemporary(compositeTex);
//}

== まとめ

基本的 (Kawase氏の発表のまま) なスターグローの実装方法について解説しましたが、
リアルタイム性にこだわらなければ、輝度画像の算出方法やパラメータを複数回切り替えることで、多用な光線を表現する方法なども考えられますね。

ここで解説した範囲でも、たとえばイテレーションのタイミングでパラメータを変更するなどすると、不均質でより "らしい", "味のある" 光線が作れるでしょう。
あるいはノイズを使って時間ごとにパラメータを変化するのもよいと思います。

物理的に正しい光線ではありませんし、より演出的で高度な光線の表現が必要なときは、ポストエフェクト以外の方法で実現することになるのでしょうが、
比較的シンプルなつくりで華やかにできるこのエフェクトもすごく面白いのでぜひ試してみてください。

…少々処理が重いですけど。

== 参照

 * Frame Buffer Postprocessing Effects in DOUBLE-S.T.E.A.L(Wreckless)
 ** http://www.daionet.gr.jp/~masa/archives/GDC2003_DSTEAL.ppt