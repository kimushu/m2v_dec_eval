//================================================================================
// 逆量子化
// $Id$
//
// tdvlc (td) から run,level ペアを順に受け取り、Inverse SCAN & 逆量子化する。
// プログレッシブのみ対応なので alt_scan = 0 固定とし、結果は内部の DP-BRAM に
// 格納する。
//
// DP-BRAM は 7 つの領域からなる。2 つは結果データ保管に交互に用い、
// 残りの 4 つが重み行列(デフォルト、スライス指定、それぞれに intra/nonintra)を
// 格納する。デフォルト側の重み行列は ROM 扱い。
// 最後の 1 つはスキャン順のインデックス配列。これも ROM 扱い。
// スキャン順は、QFS のインデックスをアドレスとして、QF のアドレス (u,v) を得る
// 配列である。
//================================================================================

module dequant(
	input clk,
	input srst,

	input slice_start,
	input [7:0] w_data,
	input w_valid,
	input mb_start,
	input blk_start,

	input mb_intra,
	input rl_valid,
	input [5:0] run,
	input signed [11:0] level

);

reg [1:0] phase_r;
reg [8:0] aaddr_r;
reg [8:0] baddr_r;
reg write_page_r;		// 今書き込む方のページ
reg wmat_intra_r;		// スライス指定行列があったとき 1
reg wmat_nonintra_r;	// スライス指定行列があったとき 1

//----------------------------------------
// ポートA側アドレス
always @(posedge clk)
	if(srst || slice_start)
		aaddr_r <= 9'h000;
	else if(w_valid)
		aaddr_r <= {2'b11, mb_intra, aaddr_r[5:0] + aaddr_r[7]};
	else if(blk_start)
		aaddr_r <= 9'h100;	// scan index 0

//----------------------------------------
// ポートB側アドレス(ぐるぐる回すだけ)
always @(posedge clk)
	if(srst || blk_start)
		baddr_r <= {2'b00, write_page_r, 6'h3f};
//	else if(..)

//----------------------------------------
// 重み係数行列(for INTRA)のフラグ
always @(posedge clk)
	if(srst || slice_start)
		wmat_intra_r <= 1'b0;
	else if(w_valid && mb_intra)
		wmat_intra_r <= 1'b1;

//----------------------------------------
// 重み係数行列(for Non-INTRA)のフラグ
always @(posedge clk)
	if(srst || slice_start)
		wmat_nonintra_r <= 1'b0;
	else if(w_valid && !mb_intra)
		wmat_nonintra_r <= 1'b1;

//----------------------------------------
// データ格納用 DP-BRAM
// addr: 9'bABCVVVUUU
//          ||| |  |
//          ||| |  +-- u-index (0-7)
//          ||| +----- v-index (0-7)
//          ||+------- page 0/1 (1:intra,0:nonintra for Wmat, 0 for scan)
//          |+-------- 0:F/scan, 1:Wmat
//          +--------- 0:default, 1:slice (0:F, 1:scan)
DPBRAM_12X512 dpbram(
	.clka  (clk),
	.wea   (),
	.addra (aaddr_r),
	.dina  (),
	.douta (),
	.clkb  (clk),
	.web   (1'b0),
	.addrb (baddr_r),
	.dinb  (12'b0),
	.doutb ()
);

endmodule

