"""
remove_white_background.py

Remove fundo branco/quase branco de imagens e salva como PNG com transparência.

Recursos de qualidade de vida:
- Se rodar sem argumentos, usa automaticamente:
    input/  -> pasta de entrada
    output/ -> pasta de saída
- Cria input/ e output/ automaticamente ao lado do script.
- Aceita imagem única ou pasta.
- Evita sobrescrever arquivos por padrão.
- Use --force para sobrescrever.
- Use --preview para gerar uma prévia com fundo quadriculado.
- Use --dry-run para mostrar o que seria feito sem gerar arquivos.
- Use --helper para modo guiado/interativo.
- Aceita input e output por argumentos posicionais OU por flags:
    -i / --input
    -o / --output
- Permite escolher o sufixo do arquivo de saída com --suffix.

Exemplos:
    python remove_white_background.py
    python remove_white_background.py --threshold 240
    python remove_white_background.py --preview
    python remove_white_background.py input/lcd.jpg
    python remove_white_background.py input/lcd.jpg output/lcd_sem_fundo.png
    python remove_white_background.py -i input/lcd.jpg -o output/lcd.png
    python remove_white_background.py --batch input output
    python remove_white_background.py -i input -o output --recursive
    python remove_white_background.py --helper
    python remove_white_background.py --examples
"""

from __future__ import annotations

import argparse
from pathlib import Path

import cv2
import numpy as np


VALID_EXTENSIONS = {".png", ".jpg", ".jpeg", ".webp", ".bmp", ".tif", ".tiff"}


def get_script_dir() -> Path:
    """Retorna a pasta em que o script está salvo."""
    return Path(__file__).resolve().parent


def ensure_default_folders(base_dir: Path) -> tuple[Path, Path]:
    """Cria as pastas padrão input/ e output/ ao lado do script."""
    input_dir = base_dir / "input"
    output_dir = base_dir / "output"

    input_dir.mkdir(parents=True, exist_ok=True)
    output_dir.mkdir(parents=True, exist_ok=True)

    return input_dir, output_dir


def is_valid_image(path: Path) -> bool:
    """Verifica se o arquivo possui extensão de imagem suportada."""
    return path.is_file() and path.suffix.lower() in VALID_EXTENSIONS


def list_images(input_path: Path, recursive: bool = False) -> list[Path]:
    """Lista imagens a partir de uma imagem única ou de uma pasta."""
    if input_path.is_file():
        return [input_path] if is_valid_image(input_path) else []

    if not input_path.exists():
        return []

    iterator = input_path.rglob("*") if recursive else input_path.iterdir()
    return sorted(path for path in iterator if is_valid_image(path))


def avoid_overwrite(path: Path) -> Path:
    """
    Evita sobrescrever arquivos existentes.

    Exemplo:
        lcd_sem_fundo.png
        lcd_sem_fundo_001.png
        lcd_sem_fundo_002.png
    """
    if not path.exists():
        return path

    parent = path.parent
    stem = path.stem
    suffix = path.suffix

    counter = 1

    while True:
        candidate = parent / f"{stem}_{counter:03d}{suffix}"
        if not candidate.exists():
            return candidate
        counter += 1


def build_output_path(
    image_path: Path,
    input_root: Path,
    output_root: Path,
    suffix: str = "_sem_fundo",
) -> Path:
    """Monta o caminho de saída, preservando subpastas quando houver."""
    if input_root.is_file():
        relative_parent = Path()
    else:
        try:
            relative_parent = image_path.parent.relative_to(input_root)
        except ValueError:
            relative_parent = Path()

    return output_root / relative_parent / f"{image_path.stem}{suffix}.png"


def create_checkerboard_preview(
    bgra_image: np.ndarray,
    square_size: int = 16,
) -> np.ndarray:
    """
    Cria uma prévia com fundo quadriculado para visualizar a transparência.

    O arquivo final com transparência continua sendo o PNG principal.
    A prévia é apenas para visualização rápida.
    """
    height, width = bgra_image.shape[:2]

    y_indices, x_indices = np.indices((height, width))
    checker = ((x_indices // square_size + y_indices // square_size) % 2) * 40 + 200
    checkerboard = np.dstack([checker, checker, checker]).astype(np.uint8)

    bgr = bgra_image[:, :, :3].astype(np.float32)
    alpha = bgra_image[:, :, 3].astype(np.float32) / 255.0
    alpha = alpha[:, :, np.newaxis]

    preview = bgr * alpha + checkerboard.astype(np.float32) * (1.0 - alpha)
    return preview.astype(np.uint8)


def remove_white_background(
    input_path: str | Path,
    output_path: str | Path,
    threshold: int = 245,
    smooth_edges: bool = True,
    force: bool = False,
    preview: bool = False,
) -> tuple[Path, Path | None]:
    """
    Remove fundo branco/quase branco de uma imagem.

    A saída principal é sempre PNG com canal alfa.
    """
    input_file = Path(input_path)
    output_file = Path(output_path)

    output_file.parent.mkdir(parents=True, exist_ok=True)

    if not force:
        output_file = avoid_overwrite(output_file)

    image = cv2.imread(str(input_file), cv2.IMREAD_COLOR)

    if image is None:
        raise FileNotFoundError(f"Não foi possível abrir a imagem: {input_file}")

    rgb = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

    white_mask = (
        (rgb[:, :, 0] >= threshold)
        & (rgb[:, :, 1] >= threshold)
        & (rgb[:, :, 2] >= threshold)
    ).astype(np.uint8) * 255

    kernel = np.ones((3, 3), np.uint8)
    white_mask = cv2.morphologyEx(white_mask, cv2.MORPH_OPEN, kernel)

    alpha = 255 - white_mask

    if smooth_edges:
        alpha = cv2.GaussianBlur(alpha, (3, 3), 0)
        alpha[alpha > 250] = 255
        alpha[alpha < 5] = 0

    b, g, r = cv2.split(image)
    result = cv2.merge([b, g, r, alpha])

    success = cv2.imwrite(str(output_file), result)

    if not success:
        raise RuntimeError(f"Não foi possível salvar a imagem em: {output_file}")

    preview_file: Path | None = None

    if preview:
        preview_file = output_file.with_name(f"{output_file.stem}_preview.png")

        if not force:
            preview_file = avoid_overwrite(preview_file)

        preview_image = create_checkerboard_preview(result)
        preview_success = cv2.imwrite(str(preview_file), preview_image)

        if not preview_success:
            raise RuntimeError(f"Não foi possível salvar a prévia em: {preview_file}")

    return output_file, preview_file


def process_images(
    input_path: Path,
    output_path: Path,
    threshold: int,
    smooth_edges: bool,
    recursive: bool,
    force: bool,
    preview: bool,
    dry_run: bool,
    suffix: str,
) -> list[tuple[Path, Path, Path | None]]:
    """Processa uma imagem ou uma pasta de imagens."""
    images = list_images(input_path, recursive=recursive)
    results: list[tuple[Path, Path, Path | None]] = []

    if not images:
        print(f"Nenhuma imagem encontrada em: {input_path}")
        return results

    for image_path in images:
        output_file = build_output_path(
            image_path=image_path,
            input_root=input_path,
            output_root=output_path,
            suffix=suffix,
        )

        if dry_run:
            print(f"[DRY-RUN] {image_path} -> {output_file}")
            results.append((image_path, output_file, None))
            continue

        generated_file, preview_file = remove_white_background(
            input_path=image_path,
            output_path=output_file,
            threshold=threshold,
            smooth_edges=smooth_edges,
            force=force,
            preview=preview,
        )

        results.append((image_path, generated_file, preview_file))

        if preview_file is None:
            print(f"OK: {image_path.name} -> {generated_file.name}")
        else:
            print(f"OK: {image_path.name} -> {generated_file.name} | preview: {preview_file.name}")

    return results


def parse_bool_answer(value: str, default: bool = False) -> bool:
    """
    Interpreta respostas simples de sim/não.

    Aceita: s, sim, y, yes, n, nao, não, no.
    Enter vazio retorna o valor default.
    """
    value = value.strip().lower()

    if value == "":
        return default

    if value in {"s", "sim", "y", "yes"}:
        return True

    if value in {"n", "nao", "não", "no"}:
        return False

    return default


def prompt_path(message: str, default: Path) -> Path:
    """Lê um caminho digitado pelo usuário, com valor padrão."""
    answer = input(f"{message} [{default}]: ").strip()
    return Path(answer) if answer else default


def prompt_int(message: str, default: int) -> int:
    """Lê um inteiro com valor padrão."""
    answer = input(f"{message} [{default}]: ").strip()
    if not answer:
        return default

    try:
        return int(answer)
    except ValueError:
        print("Valor inválido. Usando o padrão.")
        return default


def run_helper(default_input: Path, default_output: Path) -> argparse.Namespace:
    """
    Executa um modo guiado simples para facilitar o uso do script.
    """
    print("\n=== Helper: remoção de fundo branco ===")
    print("Pressione Enter para aceitar os valores padrão.\n")

    input_path = prompt_path("Caminho de entrada (arquivo ou pasta)", default_input)
    output_path = prompt_path("Caminho de saída (arquivo ou pasta)", default_output)

    threshold = prompt_int("Threshold", 245)

    preview = parse_bool_answer(input("Gerar prévia com fundo quadriculado? [n]: "), default=False)
    recursive = parse_bool_answer(input("Processar subpastas? [n]: "), default=False)
    force = parse_bool_answer(input("Sobrescrever arquivos existentes? [n]: "), default=False)
    no_smooth = parse_bool_answer(input("Desativar suavização das bordas? [n]: "), default=False)
    dry_run = parse_bool_answer(input("Executar apenas simulação (dry-run)? [n]: "), default=False)

    suffix_answer = input("Sufixo dos arquivos de saída [_sem_fundo]: ").strip()
    suffix = suffix_answer if suffix_answer else "_sem_fundo"

    return argparse.Namespace(
        input=input_path,
        output=output_path,
        threshold=threshold,
        batch=False,
        recursive=recursive,
        preview=preview,
        force=force,
        dry_run=dry_run,
        no_smooth=no_smooth,
        helper=True,
        examples=False,
        suffix=suffix,
    )


def print_examples() -> None:
    """Exibe exemplos práticos de uso."""
    print("\nExemplos de uso\n")
    print("1) Uso padrão (processa input/ e salva em output/):")
    print("   python remove_white_background.py\n")
    print("2) Ajustar threshold:")
    print("   python remove_white_background.py --threshold 240\n")
    print("3) Gerar prévia:")
    print("   python remove_white_background.py --preview\n")
    print("4) Imagem única com argumentos posicionais:")
    print("   python remove_white_background.py input/lcd.jpg output/lcd_sem_fundo.png\n")
    print("5) Imagem única com flags:")
    print("   python remove_white_background.py -i input/lcd.jpg -o output/lcd_sem_fundo.png\n")
    print("6) Pasta inteira:")
    print("   python remove_white_background.py --batch input output\n")
    print("7) Pasta com subpastas:")
    print("   python remove_white_background.py -i input -o output --recursive\n")
    print("8) Modo guiado:")
    print("   python remove_white_background.py --helper\n")


def build_parser() -> argparse.ArgumentParser:
    """Cria o parser de argumentos de linha de comando."""
    parser = argparse.ArgumentParser(
        description="Remove fundo branco/quase branco de imagens e salva PNG com transparência."
    )

    parser.add_argument(
        "input_pos",
        nargs="?",
        help="Imagem ou pasta de entrada (opcional).",
    )

    parser.add_argument(
        "output_pos",
        nargs="?",
        help="Imagem ou pasta de saída (opcional).",
    )

    parser.add_argument(
        "-i", "--input",
        dest="input_opt",
        help="Imagem ou pasta de entrada.",
    )

    parser.add_argument(
        "-o", "--output",
        dest="output_opt",
        help="Imagem ou pasta de saída.",
    )

    parser.add_argument(
        "--threshold",
        type=int,
        default=245,
        help="Sensibilidade. Menor = remove mais fundo. Padrão: 245.",
    )

    parser.add_argument(
        "--batch",
        action="store_true",
        help="Força modo de processamento em pasta.",
    )

    parser.add_argument(
        "--recursive",
        action="store_true",
        help="Busca imagens também em subpastas.",
    )

    parser.add_argument(
        "--preview",
        action="store_true",
        help="Gera uma prévia com fundo quadriculado.",
    )

    parser.add_argument(
        "--force",
        action="store_true",
        help="Sobrescreve arquivos existentes.",
    )

    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Mostra o que seria processado sem gerar arquivos.",
    )

    parser.add_argument(
        "--no-smooth",
        action="store_true",
        help="Desativa suavização das bordas.",
    )

    parser.add_argument(
        "--helper",
        action="store_true",
        help="Abre o modo guiado/interativo.",
    )

    parser.add_argument(
        "--examples",
        action="store_true",
        help="Mostra exemplos de uso e sai.",
    )

    parser.add_argument(
        "--suffix",
        default="_sem_fundo",
        help="Sufixo usado nos arquivos gerados. Padrão: _sem_fundo",
    )

    return parser


def resolve_paths(args: argparse.Namespace, default_input: Path, default_output: Path) -> tuple[Path, Path | None]:
    """
    Resolve prioridade de caminhos:
    1. flags -i/--input e -o/--output
    2. argumentos posicionais
    3. pastas padrão input/ e output/
    """
    raw_input = args.input_opt if args.input_opt is not None else args.input_pos
    raw_output = args.output_opt if args.output_opt is not None else args.output_pos

    input_path = Path(raw_input) if raw_input else default_input
    output_path = Path(raw_output) if raw_output else None

    return input_path, output_path


def main() -> None:
    """Executa o programa pela linha de comando."""
    parser = build_parser()
    args = parser.parse_args()

    base_dir = get_script_dir()
    default_input, default_output = ensure_default_folders(base_dir)

    if args.examples:
        print_examples()
        return

    if args.helper:
        args = run_helper(default_input, default_output)
        input_path = Path(args.input)
        output_arg = Path(args.output)
    else:
        input_path, output_arg = resolve_paths(args, default_input, default_output)

    smooth_edges = not args.no_smooth

    if not input_path.exists():
        print(f"Entrada não encontrada: {input_path}")
        print("Pastas padrão garantidas:")
        print(f"  input : {default_input}")
        print(f"  output: {default_output}")
        return

    is_single_image_mode = input_path.is_file() and not args.batch

    if is_single_image_mode:
        if output_arg:
            output_path = output_arg
        else:
            output_path = default_output / f"{input_path.stem}{args.suffix}.png"

        if args.dry_run:
            print(f"[DRY-RUN] {input_path} -> {output_path}")
            return

        generated_file, preview_file = remove_white_background(
            input_path=input_path,
            output_path=output_path,
            threshold=args.threshold,
            smooth_edges=smooth_edges,
            force=args.force,
            preview=args.preview,
        )

        print("\nResumo")
        print(f"Entrada             : {input_path}")
        print(f"Imagem transparente : {generated_file}")

        if preview_file:
            print(f"Prévia              : {preview_file}")

        return

    output_path = output_arg if output_arg else default_output

    results = process_images(
        input_path=input_path,
        output_path=output_path,
        threshold=args.threshold,
        smooth_edges=smooth_edges,
        recursive=args.recursive,
        force=args.force,
        preview=args.preview,
        dry_run=args.dry_run,
        suffix=args.suffix,
    )

    print("\nResumo")
    print(f"Entrada     : {input_path}")
    print(f"Saída       : {output_path}")
    print(f"Threshold   : {args.threshold}")
    print(f"Suavização  : {'sim' if smooth_edges else 'não'}")
    print(f"Recursivo   : {'sim' if args.recursive else 'não'}")
    print(f"Prévia      : {'sim' if args.preview else 'não'}")
    print(f"Dry-run     : {'sim' if args.dry_run else 'não'}")
    print(f"Sufixo      : {args.suffix}")
    print(f"Processadas : {len(results)} imagem(ns)")


if __name__ == "__main__":
    main()
