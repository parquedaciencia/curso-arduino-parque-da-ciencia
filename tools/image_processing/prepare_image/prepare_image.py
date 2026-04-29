#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path


IMAGE_EXTS = {".png", ".jpg", ".jpeg", ".webp", ".bmp", ".tif", ".tiff"}


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Pipeline paralelo para melhorar imagens e remover fundo branco.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument("input_pos", nargs="?", help="Imagem ou pasta de entrada.")
    parser.add_argument("output_pos", nargs="?", help="Imagem ou pasta de saída.")

    parser.add_argument("-i", "--input", dest="input_opt", help="Imagem ou pasta de entrada.")
    parser.add_argument("-o", "--output", dest="output_opt", help="Imagem ou pasta de saída.")

    parser.add_argument(
        "--order",
        choices=["remove-then-enhance", "enhance-then-remove"],
        default="remove-then-enhance",
        help="Ordem do pipeline.",
    )

    parser.add_argument(
        "--ensure-final-remove",
        action="store_true",
        help="Executa uma remoção de fundo extra no final para garantir transparência.",
    )

    parser.add_argument("--workers", type=int, default=max(1, min((os.cpu_count() or 2) - 1, 4)),
                        help="Número de imagens processadas em paralelo.")

    parser.add_argument("--scale", type=float, default=2.0, help="Fator de escala.")
    parser.add_argument("--width", type=int, help="Largura final em pixels.")
    parser.add_argument("--height", type=int, help="Altura final em pixels.")
    parser.add_argument("--denoise", type=int, default=3, help="Redução de ruído. Use 0 para acelerar.")
    parser.add_argument("--sharpen", type=float, default=0.8, help="Nitidez. Use 0 para acelerar.")
    parser.add_argument("--contrast", action="store_true", help="Aplica contraste local CLAHE.")
    parser.add_argument("--contrast-clip", type=float, default=2.0, help="Força do contraste CLAHE.")
    parser.add_argument("--output-ext", default=".png", help="Extensão de saída.")
    parser.add_argument("--dpi", type=int, help="DPI opcional.")

    parser.add_argument("--threshold", type=int, default=220,
                        help="Sensibilidade da remoção. Menor = remove mais fundo.")
    parser.add_argument("--no-smooth", action="store_true", help="Desativa suavização das bordas.")

    parser.add_argument("--recursive", action="store_true", help="Processa subpastas.")
    parser.add_argument("--force", action="store_true", help="Sobrescreve arquivos existentes.")
    parser.add_argument("--dry-run", action="store_true", help="Mostra comandos sem executar.")
    parser.add_argument("--keep-intermediate", action="store_true",
                        help="Mantém arquivos intermediários.")

    parser.add_argument("--final-suffix", default="_prepared", help="Sufixo do arquivo final.")

    return parser


def collect_images(input_path: Path, recursive: bool) -> list[Path]:
    if input_path.is_file():
        return [input_path]

    pattern = "**/*" if recursive else "*"
    return sorted(
        path for path in input_path.glob(pattern)
        if path.is_file() and path.suffix.lower() in IMAGE_EXTS
    )


def resolve_paths(args: argparse.Namespace, script_dir: Path) -> tuple[Path, Path]:
    input_path = Path(args.input_opt or args.input_pos or script_dir / "input")
    output_path = Path(args.output_opt or args.output_pos or script_dir / "output")
    return input_path, output_path


def final_output_path(image: Path, input_root: Path, output_root: Path, suffix: str) -> Path:
    if input_root.is_file():
        if output_root.suffix:
            return output_root
        return output_root / f"{image.stem}{suffix}.png"

    rel = image.relative_to(input_root)
    return output_root / rel.with_name(f"{rel.stem}{suffix}.png").with_suffix(".png")


def run_cmd(cmd: list[str], dry_run: bool) -> None:
    printable = " ".join(f'"{x}"' if " " in x else x for x in cmd)
    print(printable)

    if dry_run:
        return

    subprocess.run(cmd, check=True)


def remove_background_cmd(
    python_exe: str,
    script: Path,
    input_file: Path,
    output_file: Path,
    args: argparse.Namespace,
) -> list[str]:
    cmd = [
        python_exe,
        str(script),
        "-i",
        str(input_file),
        "-o",
        str(output_file),
        "--threshold",
        str(args.threshold),
        "--suffix",
        "",
    ]

    if args.force:
        cmd.append("--force")

    if args.no_smooth:
        cmd.append("--no-smooth")

    return cmd


def enhance_cmd(
    python_exe: str,
    script: Path,
    input_file: Path,
    output_file: Path,
    args: argparse.Namespace,
) -> list[str]:
    cmd = [
        python_exe,
        str(script),
        "-i",
        str(input_file),
        "-o",
        str(output_file),
        "--scale",
        str(args.scale),
        "--denoise",
        str(args.denoise),
        "--sharpen",
        str(args.sharpen),
        "--contrast-clip",
        str(args.contrast_clip),
        "--suffix",
        "",
        "--output-ext",
        args.output_ext,
    ]

    if args.width is not None:
        cmd.extend(["--width", str(args.width)])

    if args.height is not None:
        cmd.extend(["--height", str(args.height)])

    if args.contrast:
        cmd.append("--contrast")

    if args.dpi is not None:
        cmd.extend(["--dpi", str(args.dpi)])

    if args.force:
        cmd.append("--force")

    return cmd


def process_one(
    image: Path,
    input_root: Path,
    output_root: Path,
    temp_root: Path,
    scripts: dict[str, Path],
    args: argparse.Namespace,
) -> tuple[Path, bool, str]:
    python_exe = sys.executable
    final_file = final_output_path(image, input_root, output_root, args.final_suffix)

    if final_file.exists() and not args.force:
        return final_file, True, "pulada; arquivo final já existe"

    if input_root.is_file():
        rel_base = image.stem
    else:
        rel_base = str(image.relative_to(input_root).with_suffix(""))

    safe_rel = rel_base.replace("\\", "_").replace("/", "_")

    tmp1 = temp_root / f"{safe_rel}_stage1.png"
    tmp2 = temp_root / f"{safe_rel}_stage2.png"

    if not args.dry_run:
        final_file.parent.mkdir(parents=True, exist_ok=True)
        tmp1.parent.mkdir(parents=True, exist_ok=True)
        tmp2.parent.mkdir(parents=True, exist_ok=True)

    try:
        print(f"\n[PROCESSANDO] {image}")

        if args.order == "remove-then-enhance":
            run_cmd(
                remove_background_cmd(
                    python_exe, scripts["remove"], image, tmp1, args
                ),
                args.dry_run,
            )
            run_cmd(
                enhance_cmd(
                    python_exe, scripts["enhance"], tmp1, final_file, args
                ),
                args.dry_run,
            )

            if args.ensure_final_remove:
                run_cmd(
                    remove_background_cmd(
                        python_exe, scripts["remove"], final_file, tmp2, args
                    ),
                    args.dry_run,
                )

                if not args.dry_run:
                    tmp2.replace(final_file)

        else:
            run_cmd(
                enhance_cmd(
                    python_exe, scripts["enhance"], image, tmp1, args
                ),
                args.dry_run,
            )
            run_cmd(
                remove_background_cmd(
                    python_exe, scripts["remove"], tmp1, final_file, args
                ),
                args.dry_run,
            )

        return final_file, True, "ok"

    except subprocess.CalledProcessError as exc:
        return final_file, False, f"erro no subprocesso: {exc.returncode}"


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    script_dir = Path(__file__).resolve().parent
    tools_dir = script_dir.parent

    scripts = {
        "remove": tools_dir / "remove_white_background" / "remove_white_background.py",
        "enhance": tools_dir / "enhance_image" / "enhance_image.py",
    }

    for name, path in scripts.items():
        if not path.exists():
            parser.error(f"Script não encontrado ({name}): {path}")

    input_path, output_path = resolve_paths(args, script_dir)

    if not input_path.exists():
        parser.error(f"Entrada não encontrada: {input_path}")

    images = collect_images(input_path, args.recursive)

    if not images:
        print("[AVISO] Nenhuma imagem encontrada.")
        return 0

    print(f"Imagens encontradas: {len(images)}")
    print(f"Ordem do pipeline: {args.order}")
    print(f"Workers: {args.workers}")
    print(f"Threshold: {args.threshold}")
    print(f"Scale: {args.scale}")

    if args.keep_intermediate:
        temp_root = output_path / "_intermediate"
        temp_root.mkdir(parents=True, exist_ok=True)

        return run_parallel(images, input_path, output_path, temp_root, scripts, args)

    with tempfile.TemporaryDirectory(prefix="prepare_image_parallel_") as temp_dir:
        temp_root = Path(temp_dir)
        return run_parallel(images, input_path, output_path, temp_root, scripts, args)


def run_parallel(
    images: list[Path],
    input_path: Path,
    output_path: Path,
    temp_root: Path,
    scripts: dict[str, Path],
    args: argparse.Namespace,
) -> int:
    failures = 0

    with ThreadPoolExecutor(max_workers=max(1, args.workers)) as executor:
        futures = [
            executor.submit(
                process_one,
                image,
                input_path,
                output_path,
                temp_root,
                scripts,
                args,
            )
            for image in images
        ]

        for future in as_completed(futures):
            final_file, ok, msg = future.result()

            if ok:
                print(f"[OK] {final_file} — {msg}")
            else:
                failures += 1
                print(f"[ERRO] {final_file} — {msg}")

    if failures:
        print(f"\nConcluído com {failures} erro(s).")
        return 1

    print("\nPipeline concluído com sucesso.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())